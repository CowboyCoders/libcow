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

#include "cow/cow.hpp"

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

    client.start_logger();

    client.set_download_directory(dl_path);
    client.set_bittorrent_port(port_num);
	client.get_program_table();
    client.register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::on_demand_server_connection_factory()), 
        "http");
    client.register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::multicast_server_connection_factory()),
        "multicast");

    client.get_program_table();
    std::cout << "starting download" << std::endl;
    libcow::download_control* ctrl = client.start_download(1);

    if(!ctrl) {
        return 1;
    }

	//std::cout << "piece length: " << ctrl->piece_length() << std::endl;

	
    libcow::utils::buffer buf(new char[ctrl->piece_length()*100], ctrl->piece_length()*100);
    
    for(int i = 0; i < 2; ++i) {
        ctrl->get_progress();
        libcow::progress_info progress_info = ctrl->get_progress();
        std::cout << "State: " << progress_info.state_str() 
            << ", Progress: " << (progress_info.progress() * 100.0) 
            << "%" << std::endl;
        libcow::system::sleep(1000);
    }

    std::vector<libcow::piece_request> reqs;
    reqs.push_back(libcow::piece_request(ctrl->piece_length(),25,5));
    ctrl->pre_buffer(reqs);

    size_t buf_size = ctrl->piece_length() * 5;
    char* databuf = new char[buf_size];
    libcow::utils::buffer buffo(databuf, buf_size);
    ctrl->read_data(ctrl->piece_length() * 25, buffo);

    std::cout << "i'm happy!" << std::endl;

    while(true);

    delete [] databuf;

    client.stop_download(1);

    return 0;
}
