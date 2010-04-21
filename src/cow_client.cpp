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

#include <boost/thread.hpp>

using namespace libcow;

const int cow_client::default_max_num_log_messages_ = 1000;
const int cow_client::default_logging_interval_ = 1; // seconds

program_info* parse_program_info(TiXmlElement* p) {
    program_info* pi = new program_info();
    if(p->QueryIntAttribute("id", &pi->id) != TIXML_SUCCESS)
        std::cerr << "Invalid program: attribute missing\n";

    TiXmlHandle tmp_handle(p);
    TiXmlElement* child = tmp_handle.FirstChild("name").ToElement();

    pi->name = child->GetText();

    child = tmp_handle.FirstChild("description").ToElement();

    pi->description = child->GetText();
    return pi;
}


cow_client::cow_client()
    : max_num_log_messages_(default_max_num_log_messages_),
      logging_interval_(default_logging_interval_),
      download_device_id_(0)
{
    //TODO: change this to configurable log levels
	session_.set_alert_mask(libtorrent::alert::progress_notification |
							libtorrent::alert::storage_notification);

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

}

void cow_client::set_download_directory(const std::string& path)
{
    download_directory_ = path;
}

void cow_client::set_bittorrent_port(int port)
{
    session_.listen_on(std::pair<int,int>(port,port));
}

std::list<program_info> cow_client::get_program_table()
{
    download_device_information_map_.clear();

    std::list<program_info> prog_info_list;
    std::string program_table = http_get_as_string("cowboycoders.se/program_table.xml");
    BOOST_LOG_TRIVIAL(debug) << "cow_client: downloaded xml with size:" << program_table.length();
    TiXmlDocument doc;
    doc.Parse(program_table.c_str());
    if(doc.Error())
        BOOST_LOG_TRIVIAL(warning) << "cow_client: Parse error:" << doc.ErrorDesc();

    TiXmlHandle docHandle( &doc );
    // <program_id, <download_device_type, properties> >
    TiXmlElement* child = docHandle.FirstChild( "program_table" ).FirstChild( "program" ).ToElement();
    for( ; child; child=child->NextSiblingElement() )
	{
        program_info* pi(parse_program_info(child));
        if(!(pi->id)){
            continue;
        }
        TiXmlHandle tmp_handle(child);
        TiXmlElement* child2 = tmp_handle.FirstChild("download_devices").FirstChild("device").ToElement();
        std::map<std::string, properties> device_prop_map;
        for( ; child2; child2=child2->NextSiblingElement() )
        {
            std::string type;
            if(child2->QueryStringAttribute("type", &type) != TIXML_SUCCESS){
                BOOST_LOG_TRIVIAL(warning) << "cow_client: Invalid device: attribute missing";
                continue;
            }
            TiXmlHandle tmp_handle2(child2);
            TiXmlElement* child3 = tmp_handle2.FirstChild("property").ToElement();
            properties props;
            for( ; child3; child3=child3->NextSiblingElement() )
            {
                 std::string prop_name;
                 if(child3->QueryStringAttribute("name", &prop_name) != TIXML_SUCCESS){
                    BOOST_LOG_TRIVIAL(warning) << "cow_client: Invalid property: attribute missing";
                    continue;
                 }
                 props[prop_name] = child3->GetText();
            }
            device_prop_map[type] = props;
        }
        prog_info_list.push_back(*pi);
        download_device_information_map_[1] = device_prop_map;
	}
	
    return prog_info_list;
}

download_control* cow_client::get_download(int program_id)
{
    download_control_map_::iterator iter = download_controls_.find(program_id);

    if(iter == download_controls_.end()) {
        return 0;
    } else {
        return iter->second.get();
    }
}

download_control* cow_client::start_download(int program_id)
{
    libtorrent::add_torrent_params params;
    params.save_path = download_directory_;
    params.storage_mode = libtorrent::storage_mode_sparse; // allocate Ensure slot consistency. (sux)

	std::map<std::string, properties> devices = download_device_information_map_[program_id];
	
	libtorrent::torrent_handle handle;
	try{
		std::string torrent_file = http_get_as_string(devices["torrent"]["torrent"]);
		//std::ofstream File("tmp.torrent",std::ios_base::binary);
		//File << torrent_file;
		//params.tracker_url = "http://cowboycoders.se:6969/announce"; 
		params.ti = new libtorrent::torrent_info(torrent_file.data(),torrent_file.size() );
		handle = session_.add_torrent(params);
		

	} catch (libtorrent::libtorrent_exception e) {
		try{
			BOOST_LOG_TRIVIAL(debug) << "cow_client: torrent file unavailable: " << e.what(); 
			BOOST_LOG_TRIVIAL(debug) << "cow_client: trying magnet: " << devices["torrent"]["magnet"];
			params.ti = 0;
			handle = add_magnet_uri(session_, devices["torrent"]["magnet"], params);
		} catch (libtorrent::libtorrent_exception e) {
			std::cout << e.what();
		}
	}

    try {
            
        download_control_map_::iterator iter = download_controls_.find(program_id);

        download_control* dc;
        if(iter == download_controls_.end()) 
        {
            boost::shared_ptr<download_control> ptr;
            ptr.reset(new libcow::download_control(handle));
            download_controls_[program_id] = ptr;
            dc = ptr.get();
        } else {
            dc = iter->second.get();
        }

        if(dc) 
        {
            dc->piece_sources_ =  &piece_sources_;
            //TODO: Complete this stuff!
            std::map<std::string, properties>::iterator dev_iter;
            std::map<int, std::string>::iterator id_iter;
            for (dev_iter = devices.begin(); dev_iter != devices.end(); ++dev_iter) {
                int id = 0;
                for(id_iter = piece_sources_.begin(); id_iter != piece_sources_.end(); ++id_iter){
                    if(id_iter->second == dev_iter->first) {
                        id = id_iter->first;
                    }
                }
        		if(dev_iter->first != "torrent"){
                    properties props = download_device_information_map_[program_id][dev_iter->first];
                    download_device* dev = dd_manager_.create_instance(id, dev_iter->first, props);
                    if(dev) {
                        libcow::response_handler_function add_pieces_function = 
                            boost::bind(&download_control::add_pieces, dc, _1, _2);

                        dev->set_add_pieces_function(add_pieces_function);
                    }
                    dc->add_download_device(dev);
        		}
        	}
        }
        else
        {
            /* TODO: Throw exception?! */
        }

        return dc;

    } catch (libtorrent::libtorrent_exception e) {
        BOOST_LOG_TRIVIAL(warning) << "cow_client: Failed to start download with program_id "
            << program_id << ": " << e.what();
        //TODO: throw exception!
        return 0;
    }
}

void cow_client::stop_download(int program_id)
{
    download_control_map_::iterator iter = download_controls_.find(program_id);

    if(iter == download_controls_.end()) {
        BOOST_LOG_TRIVIAL(debug) << "cow_client: Could not stop download with program_id "
            << program_id << " since it is not started";
    } else {
        BOOST_LOG_TRIVIAL(debug) << "cow_client: Stopping download with program_id "
            << program_id;

        download_control* dc = iter->second.get();

        if(dc) {
            session_.remove_torrent(dc->handle_);
            download_controls_.erase(iter);
        } else {
            // TODO: throw exception!
        }
    }
}

void cow_client::logger_thread_function()
{
    while(true) {
        size_t msg_count = 0;

        std::auto_ptr<libtorrent::alert> alert_ptr = session_.pop_alert();
        libtorrent::alert* alert = alert_ptr.get();
        while(alert && msg_count < max_num_log_messages_) {
            BOOST_LOG_TRIVIAL(debug) << "cow_client (libtorrent alert): " << alert->message();
            alert_ptr = session_.pop_alert();
            alert = alert_ptr.get();
            ++msg_count;
        }
        libcow::system::sleep(logging_interval_);
    }
}

void cow_client::start_logger()
{
    logger_thread_ptr_.reset(
        new boost::thread(
            boost::bind(&cow_client::logger_thread_function, this)));

}

void cow_client::stop_logger()
{
    logger_thread_ptr_.reset();   
}

void cow_client::register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                                  const std::string& identifier)
{
    piece_sources_[++download_device_id_] = identifier;
    dd_manager_.register_download_device_factory(factory, identifier);
}
