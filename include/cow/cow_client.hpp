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
#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/future.hpp>

#include "cow/dispatcher.hpp"
#include "cow/cow_client_worker.hpp"
#include "cow/download_control.hpp"
#include "cow/exceptions.hpp"

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
        * This function will start downloading the selected program using BitTorrent.
        * If any download_devices have been registered using register_download_device_factory,
        * the client will initialize these devices and start gathering data from them as well.
        * @throws libcow::exception
        * @param program The the program to start downloading.
        * @return A pointer to the download_control used for this program.
        */
        download_control* start_download(const libcow::program_info& program, int timeout = 60)
        {
			download_control* ctrl = worker_->start_download(program, download_directory_, timeout);
			if(ctrl) {
				ctrl->set_buffering_state();
				return ctrl;
			} else {
				throw libcow::exception("Failed to start download: Could not initialize download control");
			}
        }

       /**
        * This function returns a list of all active libcow::download_controls.
        * @return A list of libcow::download_control pointers for all active downloads..
        */
        std::list<download_control*> get_active_downloads() const
        {
            return worker_->get_active_downloads();
        }

       /**
        * This function will stop the download of the specified program, and
        * erase the associated download_control.
        * @param download A pointer to the download_control instance.
        */
        void remove_download(download_control* download)
        {
            worker_->remove_download(download);
        }

       /**
        * This function registers a new download_device_factory which can be used
        * for creating new download_devices.
        * @param factory A boost::shared_ptr to the factory to register.
        * @param identifier A string describing the download_device, e.g. "http", "multicast" etc.
        */
        void register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                              const std::string& identifier)
        {
            worker_->register_download_device_factory(factory, identifier);
        }

    private:
        void alert_thread_function();
        void stop_alert_thread();
        void handle_stop_alert_thread();

        cow_client_worker* worker_;

        dispatcher* alert_disp_;

        bool alert_thread_running_;

        std::string download_directory_;

        libtorrent::session session_;
	};

    /** \example basic_example.cpp
     *  This is an example displaying how to use the libcow::cow_client API.
     *  A basic libcow::cow_client will be created that will only be able to
     *  download data using BitTorrent. In this example we will manually
     *  have to provide the info that libcow::cow_client needs in order
     *  to use BitTorrent.
     */
    
    /** \example program_table_example.cpp
     *  This is an example displaying how to use the libcow::cow_client API.
     *  A basic libcow::cow_client will be created that will only be able to
     *  download data using BitTorrent. In this example we will download
     *  the information that libcow::cow_client needs from a libcow::program_table %server,
     *  which hosts an XML file specifying all needed properties.
     */

    /** \example on_demand_example.cpp
     *  This is an example displaying how to use the libcow::cow_client API.
     *  A basic libcow::cow_client will be created that will both be able to
     *  download data using BitTorrent, and request data from an on
     *  demand %server as well. In this example we will download
     *  the information that libcow::cow_client needs from a libcow::program_table %server,
     *  which hosts an XML file specifying all needed properties.
     */

    /** \example multicast_example.cpp
     *  This is an example displaying how to use the libcow::cow_client API.
     *  A basic libcow::cow_client will be created that will both be able to
     *  download data using BitTorrent, and receive data using Multicast.
     *  In this example we will download
     *  the information that libcow::cow_client needs from a libcow::program_table %server,
     *  which hosts an XML file specifying all needed properties.
     */
}

#endif // ___libcow_cow_client___
