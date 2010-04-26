/*
Copyright 2010 CowboyCoders. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COWBOYCODERS ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COWBOYCODERS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of CowboyCoders.
*/

#include "cow/libcow_def.hpp"
#include "cow/cow_client.hpp"
#include "cow/download_control.hpp"
#include "cow/download_device.hpp"
#include "cow/curl_instance.hpp"
#include "cow/program_info.hpp"
#include "cow/system.hpp"
#include "cow/piece_data.hpp"

#include "tinyxml.h"

#include <utility>
#include <limits>

#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>

#include <boost/thread.hpp>

using namespace libcow;

const int cow_client::default_max_num_log_messages_ = 1000;
const int cow_client::default_logging_interval_ = 1; // seconds

template <typename T>
struct match_second 
{
    T val;

    match_second(T const & t) : val (t) {}

    template <typename Pair>
    bool operator() (Pair const& p) const
    {
        return (val == p.second);
    }
};

template <typename T>
struct check_pointer_value
{
    T* val;

    check_pointer_value(T* t) : val(t) {}

    bool operator() (T const& x) const
    {
        return &x == val;
    }

};

cow_client::cow_client()
    : max_num_log_messages_(default_max_num_log_messages_),
      logging_interval_(default_logging_interval_),
      logger_thread_running_(false),
      download_device_id_(0)
{
    //TODO: change this to configurable log levels
	session_.set_alert_mask(libtorrent::alert::progress_notification |
							libtorrent::alert::storage_notification | 
                            libtorrent::alert::status_notification);
    libtorrent::session_settings settings;
    settings.user_agent = "cow_agent";
    settings.allowed_fast_set_size = 1000; // 1000 pieces without choking
    settings.allow_multiple_connections_per_ip = true;
    settings.prioritize_partial_pieces = true; //TODO: might not be good, we'll see!
    //settings.seeding_piece_quota = 20;
    settings.strict_end_game_mode = false;
    settings.auto_upload_slots = false;
    session_.set_settings(settings);

    // disabling settings.auto_upload_slots and setting max_uploads to INT_MAX
    // turns all choking off
    session_.set_max_uploads(INT_MAX);

    // no limits!!
    session_.set_upload_rate_limit(0);
    session_.set_download_rate_limit(0);
    session_.set_local_upload_rate_limit(0);
    session_.set_local_download_rate_limit(0);

}

cow_client::~cow_client()
{
    stop_logger();
}

void cow_client::set_download_directory(const std::string& path)
{
    download_directory_ = path;
}

void cow_client::set_bittorrent_port(int port)
{
    session_.listen_on(std::pair<int,int>(port,port));
}

libtorrent::torrent_handle cow_client::create_torrent_handle(const properties& props)
{
    libtorrent::add_torrent_params params;
    params.save_path = download_directory_;
    params.storage_mode = libtorrent::storage_mode_allocate; // allocate Ensure slot consistency. (sux)

    properties::const_iterator torrent_it = props.find("torrent");

    if (torrent_it != props.end()) {

        const std::string& torrent = torrent_it->second;
        std::string torrent_file = http_get_as_string(torrent);

    	try {
            // Set the torrent file
    		params.ti = new libtorrent::torrent_info(torrent_file.data(), torrent_file.size());
            // Create and return the torrent_handle
    		return session_.add_torrent(params);		
    	} 
        catch (libtorrent::libtorrent_exception e) {
    		BOOST_LOG_TRIVIAL(error) << "cow_client: failed to load torrent: " << e.what(); 
    	}
    }

    properties::const_iterator magnet_it = props.find("magnet");

    if (magnet_it != props.end()) {
		try {
            // Make sure there is no torrent torrent file given
			params.ti = 0; 
            // Create a handle from a magnet uri
            return libtorrent::add_magnet_uri(session_, magnet_it->second, params);
		} 
        catch (libtorrent::libtorrent_exception e) {
            BOOST_LOG_TRIVIAL(error) << "cow_client: Error creating torrent from magnet URI: " << e.what();
		}
    }

    // If we have reached this point we have failed to create the handle
    return libtorrent::torrent_handle();
}

download_control* cow_client::start_download(const libcow::program_info& program)
{
    // Look for a torrent device
    device_map::const_iterator device_it = program.download_devices.find("torrent");

    if (device_it == program.download_devices.end()) {
        BOOST_LOG_TRIVIAL(error) << "cow_client: Failed to start download because the program does not have a torrent download device.";
        return 0;
    }

    // Fetch properties for the torrent device 
    properties torrent_props = device_it->second;

    // Create the torrent_handle from the properties
    libtorrent::torrent_handle torrent = create_torrent_handle(torrent_props);

    // Make sure the handle is valid
    if (!torrent.is_valid()) {
        BOOST_LOG_TRIVIAL(error) << "cow_client: Failed to create torrent handle.";
        return 0;
    }

    // Create a new download_control
    download_control* download = new libcow::download_control(torrent, 5, 5000); // FIXME: no magic numbers please :)
    download->piece_sources_ =  &piece_sources_;

    for (device_it = program.download_devices.begin(); 
        device_it != program.download_devices.end(); ++device_it) 
    {
        const std::string& device_type = device_it->first;
		if (device_type != "torrent") {

            // Find the <id, device_type> pair in piece_sources_ with the same type as the device
            std::map<int, std::string>::iterator piece_source_iter;
            piece_source_iter = std::find_if(piece_sources_.begin(), 
                                             piece_sources_.end(),
                                             match_second<std::string>(device_type));

            int device_id = 0;

            if (piece_source_iter != piece_sources_.end()) {
                // Piece source key is the id of the download device
                device_id = piece_source_iter->first; 

                // Get the properties for this downlod device
                const properties& props = device_it->second;

                // Create a new instance of the download device using the factory
                download_device* device = dd_manager_.create_instance(device_id, device_type, props);

                if (!device) {
                    // Write an error but continue since the at least the torrent worked
                    BOOST_LOG_TRIVIAL(error) << "cow_client: Failed to create download device of type: " << device_type;
                } else {
                    libcow::response_handler_function add_pieces_function = 
                        boost::bind(&download_control::add_pieces, download, _1, _2);

                    device->set_add_pieces_function(add_pieces_function);      
                    download->add_download_device(device);
                }

            } else {
                BOOST_LOG_TRIVIAL(warning) << "cow_client: Unsupported download device type: " << device_type;
            }

    	}
    }

    download_controls_.push_back(download);
    return download;
}

void cow_client::remove_download(download_control* download)
{
    assert(download != 0);

    download_control_vector::iterator iter = 
        std::find_if(download_controls_.begin(), download_controls_.end(), 
                     check_pointer_value<download_control>(download));

    if (iter == download_controls_.end()) {
        BOOST_LOG_TRIVIAL(warning) << "cow_client: Can't remove download since it's never registered.";
    } else {
        // Remove torrent handle
        session_.remove_torrent(download->handle_);

        // Remove from the list of controls and free memory
        download_controls_.erase(iter);

        BOOST_LOG_TRIVIAL(debug) << "cow_client: Removed program download.";
    }
}

bool cow_client::exit_logger_thread()
{
    {
        boost::mutex::scoped_lock lock(logger_mutex_);
        if(logger_thread_running_ == false) {
            return true;
        }
    }
}

void cow_client::logger_thread_function()
{
    while(true) 
    {
        if(exit_logger_thread()) {
            break;
        }

        std::auto_ptr<libtorrent::alert> alert_ptr = session_.pop_alert();
        libtorrent::alert* alert = alert_ptr.get();
        while(alert && !exit_logger_thread()) {
            if(libtorrent::hash_failed_alert* hash_alert = libtorrent::alert_cast<libtorrent::hash_failed_alert>(alert)) {
                BOOST_LOG_TRIVIAL(debug) << "cow_client: hash failed for piece: " << hash_alert->piece_index;
            } else if(libtorrent::piece_finished_alert* piece_alert = libtorrent::alert_cast<libtorrent::piece_finished_alert>(alert)) {
                BOOST_LOG_TRIVIAL(debug) << "cow_client: piece: " << piece_alert->piece_index << " was added by libtorrent"; 
            } else if(libtorrent::state_changed_alert* state_alert = libtorrent::alert_cast<libtorrent::state_changed_alert>(alert)) {
		        libtorrent::torrent_status::state_t old_state = state_alert->prev_state;
                libtorrent::torrent_status::state_t new_state = state_alert->state;
                
                if(old_state == libtorrent::torrent_status::checking_files) {
                    BOOST_LOG_TRIVIAL(debug) << "cow_client: done hashing existing files";
                }

                if(new_state == libtorrent::torrent_status::finished ||
                   new_state == libtorrent::torrent_status::seeding ||
                   new_state == libtorrent::torrent_status::downloading)
                {
                    // only emit this one time at startup,
                    // i.e. check that were not downloading and are beginning to seed
                    BOOST_LOG_TRIVIAL(debug) << "cow_client: libtorrent is up and running";
                }
                
            } else {
                BOOST_LOG_TRIVIAL(debug) << "cow_client (libtorrent alert): " << alert->message();
            }
            alert_ptr = session_.pop_alert();
            alert = alert_ptr.get();
        }
        libcow::system::sleep(logging_interval_);
    }
}

void cow_client::start_logger()
{
    {
        boost::mutex::scoped_lock lock(logger_mutex_);
        logger_thread_running_ = true;
    }
    logger_thread_ptr_.reset(
        new boost::thread(
            boost::bind(&cow_client::logger_thread_function, this)));

}

void cow_client::stop_logger()
{
    {
        boost::mutex::scoped_lock lock(logger_mutex_);
        logger_thread_running_ = false;
    }
    if(logger_thread_ptr_) {
        logger_thread_ptr_->join();
    }
}

void cow_client::register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                                  const std::string& identifier)
{
    piece_sources_[++download_device_id_] = identifier;
    dd_manager_.register_download_device_factory(factory, identifier);
}
