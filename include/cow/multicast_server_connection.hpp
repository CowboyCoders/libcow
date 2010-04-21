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

#ifndef ___libcow_multicast_server_connection___
#define ___libcow_multicast_server_connection___

#include "cow/download_device.hpp"
#include "packetizer.hpp"

#include <boost/asio.hpp>
#include <boost/utility.hpp>
#include <boost/thread.hpp>

namespace libcow {

    class LIBCOW_EXPORT multicast_server_connection 
		: public libcow::download_device
		, public boost::noncopyable
    {
    public:

        ~multicast_server_connection();
        multicast_server_connection();

        /**
         * \fn This function opens the device using the specified settings.
         * This function MUST be called in order to use the device.
         * @param settings A map of settings in the form of value pairs.
         * @return True if the device could be opened, otherwise false.
         */
         virtual bool open(const int id, const libcow::properties& settings);

        /**
         * \fn This function closes the device.
         * @return True if the device could be closed, otherwise false.
         */
         virtual bool close();

        /**
         * \fn This function returns whether or not the device is open.
         * @return True if the device is open, otherwise false.
         */
         virtual bool is_open();

        /**
         * \fn This function returns whether or not the device is readable.
         * @return True if the device is readable, otherwise false.
         */
         virtual bool is_readable();

        /**
         * \fn This function returns whether or not the device is a streaming device.
         * @return True if the device is a streaming device, otherwise false.
         */
         virtual bool is_stream();

        /**
         * \fn This function returns whether or not the device is a random access device.
         * Random access devices needs to support requests for a random piece in the media,
         * while non-random access devices simply sends the available data to the
         * download_control.
         * @return True if the device is a random access device, otherwise false.
         */
         virtual bool is_random_access();

        /**
         * \fn This function sets the callback function to call when a piece has finished
         * downloading.
         * @return True if it was possible to set the callback, otherwise false.
         */
         virtual bool set_add_pieces_function(response_handler_function add_pieces_function);

        /**
         * \fn This function is used by download_control to request pieces from random
         * access devices.
         * @return True if it was possible to get the pieces, otherwise false.
         */
         virtual bool get_pieces(const std::vector<piece_request> & requests);

    private:
         properties settings_;
         bool is_open_;
         response_handler_function handler_;
         bool is_random_access_;
         bool is_stream_;
         bool is_readable_;
         size_t current_piece_;
         size_t counter_;
         size_t packet_size_;
         size_t piece_size_;
         char * data_;
         char * piece_data_;
         libcow::packetizer packetizer_;

         size_t multicast_port_;
         std::string listen_address_;
         std::string multicast_address_;

         // pointers to threads in the thread pool
         std::vector<boost::shared_ptr<boost::thread> > threads;

         // re-ordering these will break the code!
         boost::asio::io_service io_service_;
         boost::asio::strand strand_;
         boost::asio::ip::udp::socket socket_;
         int id_;
         boost::asio::ip::udp::endpoint sender_endpoint_;
         std::ofstream file_stream_;


         void handle_receive(const boost::system::error_code& error, size_t bytes_recvd);
         void start_receive();
    };

}

#endif // ___libcow_multicast_server_connection___
