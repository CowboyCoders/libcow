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
#include "cow/multicast_server_connection.hpp"
#include "cow/piece_request.hpp"
#include "cow/piece_data.hpp"
#include "cow/utils/buffer.hpp"
#include "packetizer.hpp"

#include <fstream>
#include <cstring>
#include <iostream>

#include <boost/log/trivial.hpp>

#include <map>
#include <iterator>
#include <sstream>

using namespace libcow;

multicast_server_connection::multicast_server_connection() :
        is_open_(false),
        is_random_access_(false),
        is_stream_(true),
        is_readable_(true),
        current_piece_(11),
        counter_(0),
        packet_size_(0),
        piece_size_(0),
        packetizer_(),
        strand_(io_service_),
        socket_(io_service_),
        id_(0)
{

}
multicast_server_connection::~multicast_server_connection()
{
    delete [] data_;
    delete [] piece_data_;
    io_service_.stop();

    // wait for threads to complete their tasks
    for(size_t i = 0; i < threads.size(); ++i) {
        threads[i]->join();
    }
}
bool multicast_server_connection::open(const int id, std::string type, const libcow::properties& settings)
{
    if(is_open_) {
        BOOST_LOG_TRIVIAL(error) << "The device is already open!";
        return false;
    }
    id_ = id;
    type_ = type;
    std::string listen_addr = "";
    size_t multicast_port = 0;
    size_t packet_size = 0;
    size_t piece_size = 0;
    std::string multicast_addr = "";

    properties::const_iterator it;
    for(it = settings.begin(); it != settings.end(); ++it) {
        if(it->first.compare("listen_address") == 0) {
            listen_addr = it->second;
        } else if(it->first.compare("multicast_port") == 0) {
            std::istringstream buffer(it->second);
            buffer >> multicast_port;
        } else if(it->first.compare("multicast_address") == 0) {
            multicast_addr = it->second;
        } else if(it->first.compare("packet_size") == 0) {
            std::istringstream buffer(it->second);
            buffer >> packet_size;
        } else if(it->first.compare("piece_size") == 0) {
            std::istringstream buffer(it->second);
            buffer >> piece_size;
        }
    }

    if(listen_addr == "" ||
       multicast_addr == "" ||
       packet_size < 1 ||
       piece_size < 1 ||
       multicast_port < 1 ||
       multicast_port > 65535) {
        BOOST_LOG_TRIVIAL(error) << "Error while parsing settings.";

        return false;
    }


    const boost::asio::ip::address& listen_address(boost::asio::ip::address::from_string(listen_addr));
    const boost::asio::ip::address& multicast_address(boost::asio::ip::address::from_string(multicast_addr));
    packet_size_ = packet_size;
    piece_size_ = piece_size;

    // This will contain incoming sync- and movie_packets
    data_ = new char[packet_size_];
    memset(data_,0,packet_size);

    // This buffer will be built by stripping the movie data from the movie_packets,
    // and will eventually be sent to download_control
    piece_data_ = new char[piece_size_];
    memset(piece_data_,0,piece_size_);

    // Create the socket so that multiple may be bound to the same address.
    boost::asio::ip::udp::endpoint listen_endpoint(
            listen_address, multicast_port);
    socket_.open(listen_endpoint.protocol());
    socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));

    // Increase the buffer size of the socket (buffer size in bytes)
    // max buffer size is 262 144 bytes according to hp docs
    socket_.set_option(
            boost::asio::ip::udp::socket::receive_buffer_size(262144));
    socket_.bind(listen_endpoint);

    // Join the multicast group.
    socket_.set_option(
            boost::asio::ip::multicast::join_group(multicast_address));

    start_receive();

    boost::shared_ptr<boost::thread> thread(
            new boost::thread(
                    boost::bind(&boost::asio::io_service::run, &io_service_)));
    threads.push_back(thread);
    is_open_ = true;
    return true;
}
void multicast_server_connection::start_receive()
{
    socket_.async_receive_from(
            boost::asio::buffer(data_, packet_size_),
            sender_endpoint_,
            strand_.wrap(
                    boost::bind(&multicast_server_connection::handle_receive, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred)));

}

void multicast_server_connection::handle_receive(const boost::system::error_code& error, size_t bytes_recvd)
{
    libcow::utils::buffer buffer(data_,bytes_recvd);
    if (!error)
    {
        if(bytes_recvd > 0) {

            //BOOST_LOG_TRIVIAL(info) << "Receiving " << bytes_recvd << " bytes:";

            if(packetizer_.is_sync_packet(buffer)) {
                BOOST_LOG_TRIVIAL(info) << "Detected SYNC packet!";
                libcow::sync_packet sync = packetizer_.unpack_sync_buffer(buffer);
                BOOST_LOG_TRIVIAL(info) << "Piece id: " << sync.piece_id();

                if(sync.piece_id() != current_piece_){
                    libcow::utils::buffer piece_buffer(piece_data_,piece_size_);
                    libcow::piece_data finished_piece(current_piece_,piece_buffer);
                    current_piece_ = sync.piece_id();
                    BOOST_LOG_TRIVIAL(info) << "Sending completed piece with id " << finished_piece.index;
                    std::vector<libcow::piece_data> piece_datas;
                    counter_ = 0;
                    piece_datas.push_back(finished_piece);
                    handler_(id_, piece_datas);
                    memset(piece_data_,0,piece_size_);
                }


            } else {
                //BOOST_LOG_TRIVIAL(info) << "Detected MOVIE packet!";
                libcow::movie_packet movie = packetizer_.unpack_movie_buffer(buffer);
                //BOOST_LOG_TRIVIAL(info) << "Saving #" << counter_ << ", " << movie.data().size() << " bytes to piece_data at: " << counter_ * (packet_size_- 1);
                size_t end_byte = counter_ * (packet_size_ - 1) + movie.data().size();
                if(piece_size_ >= end_byte) {
                    memcpy(piece_data_ + counter_ * (packet_size_ - 1), movie.data().data(), movie.data().size());
                }
                ++counter_;
            }

           start_receive();

        } else {
           BOOST_LOG_TRIVIAL(info) << bytes_recvd << " bytes received in last packet";
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << error.message();
    }

}

bool multicast_server_connection::close()
{
    if(!is_open_) {
        BOOST_LOG_TRIVIAL(error) << "The device is already closed!";
        return false;
    }
#ifdef DEBUG
    BOOST_LOG_TRIVIAL(debug) << "multicast_server_connection: "
            << "waiting for worker threads to terminate...";
#endif

    io_service_.stop();

    // wait for threads to complete their tasks
    for(size_t i = 0; i < threads.size(); ++i) {
        threads[i]->join();
    }
    is_open_ = false;
    return true;
}

bool multicast_server_connection::is_open()
{
    return is_open_;
}

bool multicast_server_connection::is_readable()
{
    return is_readable_;
}

bool multicast_server_connection::is_stream()
{
    return is_stream_;
}

bool multicast_server_connection::is_random_access()
{
    return is_random_access_;
}

bool multicast_server_connection::set_add_pieces_function(response_handler_function add_pieces_function)
{
    handler_ = add_pieces_function;
    return true;
}
bool multicast_server_connection::get_pieces(const std::vector<piece_request> & requests)
{
    return true;
}
