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

   /**
    * The purpose of this class is to serve as an API that an external
    * client (e.g. a video player) might use for downloading data from
    * multiple sources. As of current, all file handling is done inside
    * libtorrent. This class can register download_device factories from
    * any source that properly implements the download_device interface,
    * and thus theoretically can be extended with new sources such as FTP,
    * local file storage etc.
    */
    class LIBCOW_EXPORT cow_client 
    {
    public:
       /**
        * Creates a new cow_client and initializes a lot of variables.
        * Since BitTorrent is used as well, choking is also disabled in
        * this constructor.
        */
        cow_client(); // TODO: Require download_directory  ?
        ~cow_client();

       /**
        * This function sets download directory to download files to. This
        * path will also be used by libtorrent.
        * @param path The path to the download directory.
        */
        void set_download_directory(const std::string& path);
       /**
        * This function sets which port to use for BitTorrent connections.
        * @param port The port to use.
        */
        void set_bittorrent_port(int port);

       /**
        * This functions downloads an XML file from a server which contains
        * information about the program table. The XML will be parsed for
        * information about which videos can be viewed in the system, and what
        * download_devices and their corresponding settings are.
        * @return A list of program_info structs, which contains information about each program.
        */
        std::list<libcow::program_info> get_program_table();

       /**
        * This function will start downloading the selected program using BitTorrent.
        * If any download_devices have been registered using register_download_device_factory,
        * the client will initialize these devices and start gathering data from them as well.
        * @param program_id The id of the program to start downloading.
        * @return A pointer to the download_control used for this program.
        */
        download_control* start_download(int program_id);

       /**
        * This function returns the download_control for a certain program.
        * @param program_id The id of the program to return a download_control for.
        * @return A pointer to the download_control for the specified program.
        */
        download_control* get_download(int program_id);

       /**
        * This function will stop the download of the specified program, and
        * erase the associated download_control.
        * @param program_id The id of the program to stop downloading.
        */
        void stop_download(int program_id);
        
       /**
        * This function starts the logger for this class in a new thread.
        */
		void start_logger();

       /**
        * This function stops the logger for this class.
        */
		void stop_logger();

       /**
        * This function registers a new download_device_factory which can be used
        * for creating new download_devices.
        * @param factory A boost::shared_ptr to the factory to register.
        * @param identifier A string describing the download_device, e.g. "http", "multicast" etc.
        */
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
