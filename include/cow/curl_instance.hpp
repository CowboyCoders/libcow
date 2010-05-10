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

#include <sstream>

#include <boost/log/trivial.hpp>
#include <boost/bind.hpp>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <cow/exceptions.hpp>
#include <cow/utils/buffer.hpp>

namespace libcow
{
    class LIBCOW_EXPORT curl_instance
    {
    public:
       /**
        * Creates a new curl_instance that make calls to the file
        * specified in the connection string.
        * @param connection_string The url to the file to make calls to.
        */
        curl_instance(const std::string& connection_string);

        ~curl_instance();

        std::stringstream& perform_unbounded_request(size_t timeout, 
                                                   const std::vector<std::string>& headers);
        utils::buffer perform_bounded_request(size_t timeout, 
                                              const std::vector<std::string>& headers,
                                              size_t buffer_size);
                           
    private:
        size_t write_allocated_data(void *downloaded_data,
                                    size_t element_size,
                                    size_t num_elements);
        size_t write_dynamic_data(void *downloaded_data,
                                  size_t element_size,
                                  size_t num_elements);
        int progress_callback(double dltotal,double dlnow,double ultotal,double ulnow);
        
        static size_t invoke_allocated_write(void *buffer,
                                             size_t element_size,
                                             size_t num_elements,
                                             void *object);
        static size_t invoke_dynamic_write(void *buffer,
                                           size_t element_size,
                                           size_t num_elements,
                                           void *object);
        static int invoke_progress_callback(void *object,
                                            double dltotal,
                                            double dlnow,
                                            double ultotal,
                                            double ulnow);
        void check_curl_code(CURLcode code);
        void set_timeout(size_t timeout);
        void set_headers(const std::vector<std::string>& headers);
        void execute_curl_request();
        
        long get_http_code() 
        {
            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_HTTP_CODE, &http_code);
            return http_code;
        }
        
        std::string url_;
        char *allocated_buffer_;
        size_t allocated_buffer_size_;
        size_t bytes_written_;
        std::stringstream dynamic_buffer_;
        CURL *curl;
    };
}

#endif // ___libcow_curl_instance___
