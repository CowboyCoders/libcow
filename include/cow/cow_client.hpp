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

#ifndef ___libcow_cow_client___
#define ___libcow_cow_client___

#include <libtorrent/session.hpp>
#include <boost/log/trivial.hpp>
#include <boost/shared_ptr.hpp>

#include "cow/download_device_manager.hpp"

namespace libcow {    

    /*
     *
     */
    class LIBCOW_EXPORT cow_client 
    {
    public:
        cow_client(); // TODO: Require download_directory  ?
        ~cow_client();

        void set_download_directory(const std::string& path);
        void set_bittorrent_port(int port);
       
        download_control* start_download(const libcow::program_info& program);

        const std::list<download_control*>& get_active_downloads() const;

        void remove_download(download_control* download);
        
		void start_logger();
		void stop_logger();

        void register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                              const std::string& identifier); 

    private:
        void logger_thread_function();

        download_device_manager dd_manager_;

        typedef std::map<int, boost::shared_ptr<libcow::download_control> >
            download_control_map_;
        
        libtorrent::session session_;
        std::string download_directory_;

		// TODO: replace the second map with multimap :D
        std::map<int, std::map<std::string, properties> > download_device_information_map_;
       
        download_control_map_ download_controls_;
    
		boost::shared_ptr<boost::thread> logger_thread_ptr_;
        bool logger_thread_running_;
        boost::mutex::scoped_lock logger_thread_lock_;

        size_t max_num_log_messages_;
        int logging_interval_;

        static const int default_max_num_log_messages_;
        static const int default_logging_interval_;
        int download_device_id_;
        std::map<int,std::string> piece_sources_;
	};
}

#endif // ___libcow_cow_client___
