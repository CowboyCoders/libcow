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

#ifndef ___libcow_curl_instance___
#define ___libcow_curl_instance___

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <boost/log/trivial.hpp>

namespace libcow
{
    /**
     * Performs a HTTP GET request.
     */
    LIBCOW_EXPORT std::string http_get_as_string(const std::string& url, size_t timeout);

    /**
     * Thrown if the internal cURL instance couldn't be initialized.
     */
     struct curl_init_exception 
         : public std::exception
     {
        /**
         * Throw the initialisation exception.
         * @return The exception.
         */
         virtual const char* what() const throw()
         {
             return "Failed to initialize curl";
         }
     };

    /**
     * A small buffer wrapper used for curl calls. Can be used for keeping
     * track of the number of bytes written to the buffer.
     */
     struct buffer_wrapper 
         : public boost::noncopyable
     {
         /**
          * Creates a new buffer_wrapper and initializes the buffer. The
          * bytes_written variable will be initialized to 0.
          * @param size The size of the buffer to use.
          */
         buffer_wrapper(size_t size)
             : buf_size(size), bytes_written(0)
         {
             buffer = new char[buf_size];
         }
     
         ~buffer_wrapper()
         {
             delete [] buffer;
         }

         /**
          * The actual data buffer.
          */
         char* buffer;

        /**
         * The size of the buffer in bytes.
         */
         size_t buf_size;

        /**
         * The number of bytes written to the buffer.
         */
         size_t bytes_written;
     };

    struct curl_instance
    {
       /**
        * Creates a new curl_instance that make calls to the file
        * specified in the connection string.
        * @param connection_string The url to the file to make calls to.
        */
        curl_instance(const std::string& connection_string)
        {
            curl = curl_easy_init();

            if(!curl) { //FIXME: how to handle this?
                throw curl_init_exception();
            }

            curl_easy_setopt(curl, CURLOPT_URL, connection_string.c_str());
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        }

        ~curl_instance()
        {
            curl_easy_cleanup(curl);
        }

       /**
        * Handle to curl.
        */
        CURL *curl;

       /**
        * libcurl callback function.
        */
        static size_t write_data(void * buffer,
                                 size_t size,
                                 size_t nmemb,
                                 void *userp);

    };
}

#endif // ___libcow_curl_instance___
