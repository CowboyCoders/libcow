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


size_t curl_instance::invoke_dynamic_write(void *downloaded_data,
                                           size_t element_size,
                                           size_t num_elements,
                                           void *object)
{
    BOOST_LOG_TRIVIAL(debug) << "curl_instance: invoke_dynamic_write called";
    curl_instance *instance = static_cast<curl_instance*>(object);
    return instance->write_dynamic_data(downloaded_data,element_size,num_elements);
}

size_t curl_instance::invoke_allocated_write(void *downloaded_data,
                                             size_t element_size,
                                             size_t num_elements,
                                             void *object)
{
    curl_instance *instance = static_cast<curl_instance*>(object);
    return instance->write_allocated_data(downloaded_data,element_size,num_elements);
}

curl_instance::curl_instance(const std::string& connection_string) :
    url_(connection_string),
    allocated_buffer_(0),
    allocated_buffer_size_(0),
    bytes_written_(0)
{
    curl = curl_easy_init();

    if(!curl) { 
       throw libcow::exception("Could not initiate curl");
    }
    curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
}

curl_instance::~curl_instance()
{
    curl_easy_cleanup(curl);
    if(allocated_buffer_ != 0) {
        delete[] allocated_buffer_;
    }
}

size_t curl_instance::write_allocated_data(void* downloaded_data, size_t element_size, size_t num_elements)
{
    if(element_size != sizeof(char)) {
        BOOST_LOG_TRIVIAL(warning) << "curl_instance::write_allocated_data: element_size is too large!";
        return 0;
    }

    if(bytes_written_ + num_elements > allocated_buffer_size_) {
        BOOST_LOG_TRIVIAL(warning) << "curl_instance::write_allocated_data: buffer overflow!";
        return 0;
    }

    for(size_t i = 0; i < num_elements; ++i) {
        allocated_buffer_[bytes_written_] = ((char *) downloaded_data)[i];
        bytes_written_++;
    }

    return element_size*num_elements;
}

size_t curl_instance::write_dynamic_data(void* downloaded_data, size_t element_size, size_t num_elements)
{
    BOOST_LOG_TRIVIAL(debug) << "curl_instance: writing dynamic data";
    for(size_t i = 0; i < element_size*num_elements; ++i) {
        dynamic_buffer_ << ((char *) downloaded_data)[i];
    }

    return element_size*num_elements;
}

void curl_instance::check_curl_code(CURLcode code)
{
    if(code != CURLE_OK) {
        std::stringstream msg;
        msg << "Failed to set curl option. curl error: " << curl_easy_strerror(code);
        throw libcow::exception(msg.str());
    }
}

void curl_instance::set_timeout(size_t timeout)
{
    CURLcode res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    check_curl_code(res);
}

void curl_instance::set_headers(const std::vector<std::string>& headers)
{
    struct curl_slist *chunk = 0;
    std::vector<std::string>::const_iterator it;
    for(it = headers.begin(); it != headers.end(); ++it) {
        const char *header = it->c_str();
        chunk = curl_slist_append(chunk,header);
    }
   
    if(chunk != 0) {
        CURLcode res = curl_easy_setopt(curl,CURLOPT_HTTPHEADER,chunk);
        check_curl_code(res);
    }
}

void curl_instance::execute_curl_request()
{
    BOOST_LOG_TRIVIAL(debug) << "curl_instance: execute_curl_request called";
    CURLcode res = curl_easy_perform(curl);
    
    if(res != CURLE_OK) {
        std::stringstream msg;
        msg << "Download failed from URL '" << url_ << "': " << curl_easy_strerror(res);
        throw libcow::exception(msg.str());
    }

    long http_code = get_http_code();
    
    if(http_code != 200) {
        std::stringstream msg;
        msg << "Download failed from URL '" << url_ << "'. Error code: " << http_code;
        throw libcow::exception(msg.str());
    }
}

utils::buffer curl_instance::perform_bounded_request(size_t timeout, 
                                                     const std::vector<std::string>& headers,
                                                     size_t buffer_size)
{
    set_timeout(timeout);
    set_headers(headers);
    
    allocated_buffer_size_ = buffer_size;
    allocated_buffer_ = new char[allocated_buffer_size_];

    CURLcode res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_instance::invoke_allocated_write); 
    check_curl_code(res);
    
    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    check_curl_code(res);
    
    execute_curl_request();
    
    return utils::buffer(allocated_buffer_,allocated_buffer_size_);
}

std::stringstream& curl_instance::perform_unbounded_request(size_t timeout, 
                                                            const std::vector<std::string>& headers)
{
    BOOST_LOG_TRIVIAL(debug) << "curl_instance: performing an unbounded request";
    set_timeout(timeout);
    set_headers(headers);
    
    
    CURLcode res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_instance::invoke_dynamic_write); 
    check_curl_code(res);
    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    check_curl_code(res);
    
    execute_curl_request();
    
    return dynamic_buffer_;
}
