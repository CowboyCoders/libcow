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

#include <iostream>
#include <map>
#include <boost/function.hpp>
#include <boost/log/trivial.hpp>
#include <libtorrent/hasher.hpp>
#include <cow/cow.hpp>


void add_pieces_callback(int id, std::vector<libcow::piece_data> pieces) {
    BOOST_LOG_TRIVIAL(info) << "Receiving pieces: ";
    std::vector<libcow::piece_data>::iterator it;
    for(it = pieces.begin(); it != pieces.end(); ++it) {
        BOOST_LOG_TRIVIAL(info) << "Index: " << it->index << " Size: " << it->data.size();
        libtorrent::hasher hash(it->data.data(),it->data.size());
        BOOST_LOG_TRIVIAL(info) << "Hash: " << hash.final();
        hash.reset();
    }
}

void print_connection_state(libcow::on_demand_server_connection & connection) {
    BOOST_LOG_TRIVIAL(info) << "Connection state: ";
    BOOST_LOG_TRIVIAL(info) << "Open: " << connection.is_open() << " Readable: " << connection.is_readable();
    BOOST_LOG_TRIVIAL(info) << "Random Access: " << connection.is_random_access() << " Stream: " << connection.is_stream();

}

int main()
{
    // INTEGRATION TESTING GUIDELINES:
    // * add test classes to this project
    // * run all tests in this main function
    // * use assert (#include <cassert>) for assertions

    libcow::on_demand_server_connection test_connection;
    std::cerr << "Logging all output to download_device_tests.log!" << std::endl;

    print_connection_state(test_connection);
    BOOST_LOG_TRIVIAL(info) << "Opening connection...";

    libcow::properties settings;
    settings["address"] = "cowboycoders.se";
    settings["port"] = "22234";
    settings["file"] = "big_buck_bunny.mpg";
    settings["max_simultaneous_downloads"] = "3";

    test_connection.open(1, "http", settings);
    print_connection_state(test_connection);

    test_connection.set_add_pieces_function(boost::bind(&add_pieces_callback,_1,_2));

    std::vector<libcow::piece_request> requests;

    for(int i = 0; i < 10; i += 1) {
        requests.push_back(libcow::piece_request(150+i,i,1));
    }

    test_connection.get_pieces(requests);

    while(true) {
        BOOST_LOG_TRIVIAL(info) << "Waiting...";
        
        libcow::system::sleep(100);
    }


    BOOST_LOG_TRIVIAL(info) << "Success";

    return 0;
}

