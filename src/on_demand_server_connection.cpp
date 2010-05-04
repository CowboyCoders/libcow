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
#include "cow/on_demand_server_connection.hpp"
#include "cow/piece_request.hpp"
#include "cow/piece_data.hpp"

#include <boost/log/trivial.hpp>

#include <iterator>
#include <sstream>

using namespace libcow;

on_demand_server_connection::on_demand_server_connection() :
        is_open_(false),
        is_random_access_(true),
        is_stream_(false),
        is_readable_(true),
        work(io_service),
        id_(0)
{

}

on_demand_server_connection::~on_demand_server_connection()
{
    BOOST_LOG_TRIVIAL(debug) << "on_demand_server_connection: "
            << "waiting for worker threads to terminate...";
    io_service.stop();

    // wait for threads to complete their tasks
    for(size_t i = 0; i < threads.size(); ++i) {
        threads[i]->join();
    }

}

bool on_demand_server_connection::open(const int id, const properties & settings)
{
    if(is_open_) {
        BOOST_LOG_TRIVIAL(error) << "The device is already open!";
        return false;
    }
    id_ = id;
    std::string address = "";
    size_t port = 0;
    std::string file = "";
    size_t max_simultaneous_downloads = 0;

    properties::const_iterator it;
    for(it = settings.begin(); it != settings.end(); ++it) {
        if(it->first.compare("address") == 0) {
            address = it->second;
        } else if(it->first.compare("port") == 0) {
            std::istringstream buffer(it->second);
            buffer >> port;
        } else if(it->first.compare("file") == 0) {
            file = it->second;
        } else if(it->first.compare("max_simultaneous_downloads") == 0) {
            std::istringstream buffer(it->second);
            buffer >> max_simultaneous_downloads;
        }
    }
    if(address == "" ||
       file == "" ||
       port < 1 ||
       port > 65535 ||
       max_simultaneous_downloads < 1) {
       BOOST_LOG_TRIVIAL(error) << "Error while parsing settings.";

        return false;
    }
    std::stringstream ss;
    ss << address << ":" << port << "/" << file;
    connection_string_ = ss.str();

    for(size_t i = 0; i < max_simultaneous_downloads; ++i) {
        boost::shared_ptr<boost::thread> thread(
                new boost::thread(
                        boost::bind(&boost::asio::io_service::run, &io_service)));
        threads.push_back(thread);
    }


    settings_ = settings;
    is_open_ = true;
    return true;
}

bool on_demand_server_connection::close()
{
    if(!is_open_) {
        BOOST_LOG_TRIVIAL(error) << "The device is already closed!";
        return false;
    }
#ifdef DEBUG
    BOOST_LOG_TRIVIAL(debug) << "on_demand_server_connection: "
            << "waiting for worker threads to terminate...";
#endif

    io_service.stop();

    // wait for threads to complete their tasks
    for(size_t i = 0; i < threads.size(); ++i) {
        threads[i]->join();
    }
    is_open_ = false;
    return true;
}

void on_demand_server_connection::send(size_t piece_size, std::vector<int> indices)
{
    io_service.post(
            boost::bind(&on_demand_server_connection::worker,
                        this,
                        connection_string_,
                        piece_size,
                        indices));
    
}

size_t libcow::curl_instance::write_data(void * buffer,
                                         size_t size,
                                         size_t nmemb,
                                         void *userp)
{
    if(size != 1) {
        BOOST_LOG_TRIVIAL(debug) << "on_demand_server_connection: error: "
                << "chunk size not 1";
        return 0;
    }

    libcow::buffer_wrapper * wrapper = (libcow::buffer_wrapper*)userp;

    if(wrapper->bytes_written + nmemb > wrapper->buf_size) {
        BOOST_LOG_TRIVIAL(debug) << "on_demand_server_connection: error: "
                << "buffer overflow";
        return 0;
    }

    for(size_t i = 0; i < nmemb; ++i) {
        wrapper->buffer[wrapper->bytes_written++] = ((char*)buffer)[i];
    }

    return size * nmemb;
}


void on_demand_server_connection::worker(std::string connection_str,
                                         const std::size_t piece_size,
                                         const std::vector<int> indices)
{
    // create a new curl instance for this thread if none exists
    if(curl_ptr.get() == 0) {
        curl_ptr.reset(new curl_instance(connection_str));
        // please note that the curl_instance will be deleted automatically
        // when this thread terminates.
    }

    /* allocate a fat buffer for all pieces to be downloaded.
        * buffer_wrapper will delete its internal buffer when we fall out
        * of this scope.
        */
    libcow::buffer_wrapper wrapper(piece_size * indices.size());

    // these are the pointers we will actually pass to the user
    std::vector<piece_data> piece_datas;

    // "slice" up the buffer into pieces
    // (in fact, we just do some pointer arithmetic on the buffer)
    for(size_t i = 0; i < indices.size(); ++i) {
        piece_datas.push_back(
                piece_data(indices[i], utils::buffer(wrapper.buffer + (piece_size * i), piece_size)));
    }

    curl_easy_setopt(curl_ptr->curl, CURLOPT_WRITEDATA, (void*)&wrapper);
    curl_easy_setopt(curl_ptr->curl, CURLOPT_TIMEOUT, 60); // 60 seconds time limit for request

    std::stringstream size_str;
    size_str << "Size: " << piece_size;

    std::stringstream index_str;
    index_str << "Indices: ";

    std::vector<int>::const_iterator it;
    for(it = indices.begin(); it != indices.end(); ++it) {
        if((it+1) == indices.end())
        {
            index_str << *it;
        } else {
            index_str << (*it) << " ";
        }
    }

    BOOST_LOG_TRIVIAL(debug) << "Size: " << size_str.str();
    BOOST_LOG_TRIVIAL(debug) << "Indices: " << index_str.str();

    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, size_str.str().c_str());
    chunk = curl_slist_append(chunk, index_str.str().c_str());

    CURLcode res;

    res = curl_easy_setopt(curl_ptr->curl, CURLOPT_HTTPHEADER, chunk);


    if(res != CURLE_OK) {
        BOOST_LOG_TRIVIAL(debug) << "on_demand_server_connection: error: "
                << curl_easy_strerror(res);
        return;
    }

    res = curl_easy_perform(curl_ptr->curl);

    if(res != CURLE_OK) {
        BOOST_LOG_TRIVIAL(debug) << "on_demand_server_connection: error: "
                << curl_easy_strerror(res);
        return;
    }

    // invoke user defined callback
    if(handler_ == NULL){
        BOOST_LOG_TRIVIAL(error) << "No add pieces function set!";
        return;
    }
    if(handler_) {
        handler_(id_, piece_datas);
    }

}

bool on_demand_server_connection::is_open()
{
    return is_open_;
}

bool on_demand_server_connection::is_readable()
{
    return is_readable_;
}

bool on_demand_server_connection::is_stream()
{
    return is_stream_;
}

bool on_demand_server_connection::is_random_access()
{
    return is_random_access_;
}

bool on_demand_server_connection::set_add_pieces_function(response_handler_function add_pieces_function)
{
    handler_ = add_pieces_function;
    return true;
}


bool on_demand_server_connection::get_pieces(const std::vector<piece_request> & requests)
{
    if(!is_open_){
        BOOST_LOG_TRIVIAL(error) << "The device is not open!";
        return false;
    }

    if(requests.size() == 0)
    {
        BOOST_LOG_TRIVIAL(error) << "Request input was of size 0";
        return false;
    } else
    {
        std::vector<int> indices;

        std::vector<piece_request>::const_iterator it;
        for(it = requests.begin(); it != requests.end(); ++it)
        {
            piece_request req = *it;
            for(size_t i = 0; i < req.count; ++i)
            {
                indices.push_back(req.index+i);
            }
        }

        send(requests[0].piece_size, indices);
        return true;
    }
}
