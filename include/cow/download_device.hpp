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

#ifndef ___libcow_download_device___
#define ___libcow_download_device___

#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

namespace libcow {

   /**
    * \class download_device 
    * A download_device serves as an abstraction layer between different
    * download sources and a download_control. This interface can be
    * implemented in order to create new download sources,
    * e.g. BitTorrent, FTP, Multicast etc.
    */
    class LIBCOW_EXPORT download_device
    {
    public:
        
       virtual ~download_device() {}

       /** 
        * This function opens the device using the specified settings.
        * This function MUST be called in order to use the device.
        * @param id The unique id for this device.
        * @param type The type for this download_device
        * @param settings A map of settings in the form of value pairs.
        * @return True if the device could be opened, otherwise false.
        */
        virtual bool open(const int id, std::string type, const libcow::properties& settings) = 0;

       /**
        * This function closes the device.
        * @return True if the device could be closed, otherwise false.
        */
        virtual bool close() = 0;

       /**
        * This function returns whether or not the device is open.
        * @return True if the device is open, otherwise false.
        */
        virtual bool is_open() = 0;

       /**
        * This function returns whether or not the device is readable.
        * @return True if the device is readable, otherwise false.
        */
        virtual bool is_readable() = 0;

       /**
        * This function returns whether or not the device is a streaming device.
        * @return True if the device is a streaming device, otherwise false.
        */
        virtual bool is_stream() = 0;

       /**
        * This function returns whether or not the device is a random access device.
        * Random access devices needs to support requests for a random piece in the media,
        * while non-random access devices simply sends the available data to the
        * download_control.
        * @return True if the device is a random access device, otherwise false.
        */
        virtual bool is_random_access() = 0;

       /**
        * This function sets the callback function to call when a piece has finished
        * downloading.
        * This function MUST be called in order to use the device.
        * @param add_pieces_function The callback to send completed pieces to.
        * @return True if it was possible to set the callback, otherwise false.
        */
        virtual bool set_add_pieces_function(libcow::response_handler_function add_pieces_function) = 0;

       /**
        * This function is used by download_control to request pieces from random
        * access devices.
        * @param requests A vector of requested pieces.
        * @return True if it was possible to get the pieces, otherwise false.
        */
        virtual bool get_pieces(const std::vector<libcow::piece_request> & requests) = 0;

        /**
         * This function returns the id for this download control.
         *
         * @return The id for the device
         */
        virtual int id() const = 0;

        /**
         * This functions returns the description for this download control
         *
         * @return A string describing this download device
         */
        virtual std::string type() const = 0;

    private:

    };

}

#endif // ___libcow_download_device___
