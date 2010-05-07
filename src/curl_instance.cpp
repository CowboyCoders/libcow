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
#include "cow/curl_instance.hpp"
#include "cow/exceptions.hpp"

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

using namespace libcow;

/* Casting horror stringstream recipient for curl. But it works. */
size_t stringstream_write_data(void* buffer, size_t siz, size_t nmemb, void* data)
{
    for(size_t i = 0; i < siz*nmemb; ++i) {
        (*((std::stringstream*) data)) << ((char*) buffer)[i];
    }
    return siz*nmemb;
}

/*
 * HTTP GET -> std::string
 */
std::string libcow::http_get_as_string(const std::string& url,size_t timeout)
{
    curl_instance curl(url);
    CURLcode res;

    // Set callback
    std::stringstream ss;

    curl_easy_setopt(curl.curl, CURLOPT_WRITEDATA, &ss);
    curl_easy_setopt(curl.curl, CURLOPT_WRITEFUNCTION, &stringstream_write_data);
    curl_easy_setopt(curl.curl, CURLOPT_TIMEOUT, timeout);

    res = curl_easy_perform(curl.curl);

    if(res != CURLE_OK) {
        std::stringstream msg;
        msg << "Download failed. " << curl_easy_strerror(res);
        throw libcow::exception(msg.str());
    }

    return ss.str(); //std::string(ss.str());
}
