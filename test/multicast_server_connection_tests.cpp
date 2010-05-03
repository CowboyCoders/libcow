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

#include <cow/cow.hpp>
#include <cow/multicast_server_connection.hpp>

#include <boost/log/trivial.hpp>
#include <libtorrent/hasher.hpp>


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

int main()
{
    // INTEGRATION TESTING GUIDELINES:
    // * add test classes to this project
    // * run all tests in this main function
    // * use assert (#include <cassert>) for assertions
    libcow::multicast_server_connection test_connection;
    std::cerr << "Logging all output to multicast_server_connection_tests.log!" << std::endl;

    BOOST_LOG_TRIVIAL(info) << "Opening connection...";

    libcow::properties settings;
    settings["listen_address"] = "0.0.0.0";
    settings["multicast_port"] = "12345";
    settings["multicast_address"] = "224.0.100.100";
    settings["packet_size"] = "1024";
    settings["piece_size"] = "262144";

    test_connection.open(1, settings); // It seems that we must open the device before setting the callback

    test_connection.set_add_pieces_function(boost::bind(&add_pieces_callback,_1,_2));

    while(true) {
        BOOST_LOG_TRIVIAL(info) << "Waiting...";
        libcow::system::sleep(1000);
    }

    return 0;
}

