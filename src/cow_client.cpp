/*
Copyright 2010 CowboyCoders. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COWBOYCODERS ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL COWBOYCODERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of CowboyCoders.
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

// DANIEL, these should use some kind of namespace! 
// Reason: for clarity, it seems to have them without namespace
// you could also make them private, that's your choice...
// DANIEL, these should use some kind of namespace! 
// Reason: for clarity, it seems to have them without namespace
// you could also make them private, that's your choice...
cow_client::cow_client()
{
    //TODO: change this to configurable log levels
	session_.set_alert_mask(libtorrent::alert::progress_notification |
							libtorrent::alert::storage_notification | 
                            libtorrent::alert::status_notification);
    libtorrent::session_settings settings;
    settings.user_agent = "libcow";
    settings.allowed_fast_set_size = 1000; // 1000 pieces without choking
    settings.allow_multiple_connections_per_ip = true;
    settings.prioritize_partial_pieces = true; //TODO: might not be good, we'll see!
    settings.seeding_piece_quota = 20;
    settings.strict_end_game_mode = false;
    settings.auto_upload_slots = false;
    session_.set_settings(settings);

    // disabling settings.auto_upload_slots and setting max_uploads to INT_MAX
    // turns all choking off
    session_.set_max_uploads(INT_MAX);

    // no limits!! YEEHA!
    session_.set_upload_rate_limit(0);
    session_.set_download_rate_limit(0);
    session_.set_local_upload_rate_limit(0);
    session_.set_local_download_rate_limit(0);

    /* Please note that alert_disp_ uses worker_, 
     * so make sure to not start alert_thread_function
     * before the worker is created!
     */
    worker_ = new cow_client_worker(session_);
    alert_disp_ = new dispatcher(0);

    alert_thread_running_ = true;
    alert_disp_->post(boost::bind(
        &cow_client::alert_thread_function, this));
    
}

cow_client::~cow_client()
{
    stop_alert_thread();
    
    /* Please note that alert_disp_ uses worker_, 
     * so make sure to delete them in the right order!
     */
    delete alert_disp_;
    delete worker_;
}

void cow_client::set_download_directory(const std::string& path)
{
    download_directory_ = path;
}

void cow_client::set_bittorrent_port(int port)
{
    session_.listen_on(std::pair<int,int>(port,port));
}

void cow_client::alert_thread_function()
{
    if(alert_thread_running_) 
    {
        std::auto_ptr<libtorrent::alert> alert_ptr = session_.pop_alert();
        libtorrent::alert* alert = alert_ptr.get();
        while(alert) {
            if(libtorrent::hash_failed_alert* hash_alert = libtorrent::alert_cast<libtorrent::hash_failed_alert>(alert)) {
                BOOST_LOG_TRIVIAL(debug) << "cow_client: hash failed for piece: " 
                                         << hash_alert->piece_index;
            } else if(libtorrent::piece_finished_alert* piece_alert = libtorrent::alert_cast<libtorrent::piece_finished_alert>(alert)) {
                BOOST_LOG_TRIVIAL(debug) << "cow_client: piece: " 
                                         << piece_alert->piece_index 
                                         << " was added by libtorrent"; 
                worker_->signal_piece_finished(piece_alert->handle, piece_alert->piece_index);
            } else if(libtorrent::state_changed_alert* state_alert = libtorrent::alert_cast<libtorrent::state_changed_alert>(alert)) {
		        libtorrent::torrent_status::state_t old_state = state_alert->prev_state;
                libtorrent::torrent_status::state_t new_state = state_alert->state;
                
                if((old_state == libtorrent::torrent_status::checking_files ||
                    old_state == libtorrent::torrent_status::checking_resume_data ||
                    old_state == libtorrent::torrent_status::allocating) 
                    &&
                   (new_state == libtorrent::torrent_status::finished ||
                    new_state == libtorrent::torrent_status::seeding ||
                    new_state == libtorrent::torrent_status::downloading))
                {
                    // we're done hashing files and are now ready to seed or download.
                    BOOST_LOG_TRIVIAL(debug) << "cow_client: libtorrent is up and running";
                    worker_->signal_startup_complete(state_alert->handle);
                }
                
            } else {
                // BOOST_LOG_TRIVIAL(debug) << "cow_client (libtorrent alert): " << alert->message();
            }
            alert_ptr = session_.pop_alert();
            alert = alert_ptr.get();
        }
        libcow::system::sleep(100);
        //loop
        alert_disp_->post(boost::bind(&cow_client::alert_thread_function, this));
    }
}

void cow_client::stop_alert_thread()
{
    alert_disp_->post(boost::bind(&cow_client::handle_stop_alert_thread, this));
}

// invoked by alert_disp_
void cow_client::handle_stop_alert_thread()
{
    alert_thread_running_ = false;
}

