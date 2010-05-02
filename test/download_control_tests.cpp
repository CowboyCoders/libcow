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
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTrWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of CowboyCoders.
*/


#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/shared_ptr.hpp>
#include <libtorrent/session.hpp>

#include <cow/cow.hpp>

void invoked_after_init() 
{
    std::cout << "Init done!" << std::endl;
}

void got_wanted_pieces()
{
    std::cout << "Got wanted pieces!" << std::endl;
}

int main(int argc, char* argv[])
{
    if(argc != 3) {
        std::cout << "usage: download_control_test PORT_NUMBER DOWNLOAD_PATH" << std::endl;
        return 1;
    }

    int port_num = atoi(argv[1]);
    std::string dl_path = argv[2];

    BOOST_LOG_TRIVIAL(debug) << "Starting download_control test";

    libcow::cow_client client;

    client.set_download_directory(dl_path);
    client.set_bittorrent_port(port_num);
    client.register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::on_demand_server_connection_factory()), 
        "http");
    client.register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::multicast_server_connection_factory()),
        "multicast");

    libcow::program_table prog_table;
    prog_table.load_from_http("cowboycoders.se/program_table.xml",120);

    std::cout << "starting download" << std::endl;
    libcow::download_control* ctrl = client.start_download(prog_table.at(0));

    if(!ctrl) {
        return 1;
    }

    ctrl->invoke_after_init(boost::bind(invoked_after_init));

    std::vector<int> must_have;
    must_have.push_back(1);
    must_have.push_back(2);
    must_have.push_back(5);

    ctrl->invoke_when_downloaded(must_have, boost::bind(got_wanted_pieces));

    std::cout << "testing blocking read...." << std::endl;

    std::vector<libcow::piece_request> reqs1;
    reqs1.push_back(libcow::piece_request(ctrl->piece_length(), 0, 1));
    ctrl->pre_buffer(reqs1);

    std::vector<libcow::piece_request> reqs2;
    reqs2.push_back(libcow::piece_request(ctrl->piece_length(), 1, 1));
    ctrl->pre_buffer(reqs2);

    std::vector<libcow::piece_request> reqs3;
    reqs3.push_back(libcow::piece_request(ctrl->piece_length(), 2, 1));
    ctrl->pre_buffer(reqs3);

    std::vector<libcow::piece_request> reqs4;
    reqs4.push_back(libcow::piece_request(ctrl->piece_length(), ctrl->num_pieces()-1, 1));
    ctrl->pre_buffer(reqs4);

	//std::cout << "piece length: " << ctrl->piece_length() << std::endl;

    char* testbuf = new char[16*1024];
    libcow::utils::buffer testbuf_wrap(testbuf, 16*1024);
    char* testbuf2 = new char[4064];
    libcow::utils::buffer testbuf_wrap2(testbuf2, 4064);

    //FIXME: this test depends on file being big_buck_bunny.mpg
    std::cout << "read attempt 1: " << ctrl->read_data(150118368, testbuf_wrap2) << std::endl;
    std::cout << "read attempt 2: " << ctrl->read_data(0, testbuf_wrap) << std::endl;
    std::cout << "read attempt 3: " << ctrl->read_data(0, testbuf_wrap) << std::endl;

    libcow::utils::buffer buf(new char[ctrl->piece_length()*100], ctrl->piece_length()*100);

    for(size_t i = 0; i < 400; ++i) {
 
        //ctrl->debug_print();

        size_t playback_pos = i*256*1024;
        std::cout << "setting playback position to " << playback_pos << std::endl;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(250);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(250);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(250);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(250);
    }

    //client.remove_download(ctrl);

    return 0;
}
