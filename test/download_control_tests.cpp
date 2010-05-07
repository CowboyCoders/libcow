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
#include <boost/thread.hpp>
#include <cow/cow.hpp>
#include <cassert>

boost::mutex mutex;
boost::condition_variable cond;

void invoked_after_init() 
{
    
    std::cout << "Init done!" << std::endl;
    cond.notify_one();
}

void got_wanted_pieces()
{
    std::cout << "Got wanted pieces!" << std::endl;
}

void piece_finished_callback(int piece_index, int device)
{
    std::cout << "Piece finished: " << piece_index << " from device: " << device << std::endl;
}

void print_device_map(std::map<int,std::string> devices)
{
    std::map<int,std::string>::iterator it;
    for(it = devices.begin(); it != devices.end(); ++it) {
        std::pair<int,std::string> ele = *it;
        std::cout << "device with id: " << ele.first << " is called " << ele.second << std::endl;
    }
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

    libcow::cow_client* client = new libcow::cow_client();

    client->set_download_directory(dl_path);
    client->set_bittorrent_port(port_num);
    client->register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::on_demand_server_connection_factory()), 
        "http");
    client->register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::multicast_server_connection_factory()),
        "multicast");

    libcow::program_table prog_table;
    try 
    {
        prog_table.load_from_http("localhost:8080/program_table.xml",120);
    }
    catch (libcow::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    if(prog_table.size() == 0) {
        std::cout << "Couldn't fetch program table!" << std::endl;
        return 1;
    }

    // tesqzt get_active_downloads
    std::list<libcow::download_control*> active_downloads 
        = client->get_active_downloads();

    assert(active_downloads.size() == 0);

    std::cout << "starting download" << std::endl;
    
    libcow::download_control* ctrl;
 
    if(prog_table.size() < 1) {
        std::cerr << "Error: Got empty program table." << std::endl;
        return 1;
    }
    libcow::program_info& proginfo = prog_table.at(0);
    
    try 
    {
        ctrl = client->start_download(proginfo);
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }


    ctrl->set_piece_finished_callback(piece_finished_callback);

    ctrl->invoke_after_init(boost::bind(invoked_after_init));


    boost::unique_lock<boost::mutex> lock(mutex);
    cond.wait(lock);
    
    std::vector<int> state;
    ctrl->get_current_state(state);

    std::vector<int>::iterator it;
    std::cout << "CURRENT STATE" << std::endl;
    for(it = state.begin(); it != state.end(); ++it)
    {
        std::cout << *it;
    }
    std::cout << std::endl;

    std::vector<int> must_have;
    must_have.push_back(0);
    must_have.push_back(1);
    must_have.push_back(2);
    
    std::map<int,std::string> dmap = ctrl->get_device_names();
    print_device_map(dmap);

    ctrl->invoke_when_downloaded(must_have, boost::bind(got_wanted_pieces));

    std::cout << "testing blocking read...." << std::endl;

    std::vector<int> reqs1;
    reqs1.push_back(0);
    ctrl->pre_buffer(reqs1);

    std::vector<int> reqs2;
    reqs2.push_back(1);
    ctrl->pre_buffer(reqs2);

    std::vector<int> reqs3;
    reqs3.push_back(2);
    ctrl->pre_buffer(reqs3);

    std::vector<int> reqs4;
    reqs4.push_back(ctrl->num_pieces() - 1);
    ctrl->pre_buffer(reqs4);

	//std::cout << "piece length: " << ctrl->piece_length() << std::endl;

    char* testbuf = new char[16*1024];
    libcow::utils::buffer testbuf_wrap(testbuf, 16*1024);
    char* testbuf2 = new char[4064];
    libcow::utils::buffer testbuf_wrap2(testbuf2, 4064);

    //FIXME: this test depends on file being big_buck_bunny.mpg
    //std::cout << "read attempt 1: " << ctrl->read_data(150118368, testbuf_wrap2) << std::endl;
    //std::cout << "read attempt 2: " << ctrl->read_data(0, testbuf_wrap) << std::endl;
    //std::cout << "read attempt 3: " << ctrl->read_data(0, testbuf_wrap) << std::endl;

    libcow::utils::buffer buf(new char[ctrl->piece_length()*100], ctrl->piece_length()*100);

    for(size_t i = 0; i < 10; ++i) {
 
        //ctrl->debug_print();

        size_t playback_pos = i*256*1024;
        std::cout << "setting playback position to " << playback_pos << std::endl;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(250);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(150);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(50);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(10);
    }

    // test get_active_downloads
    active_downloads 
        = client->get_active_downloads();

    assert(active_downloads.size() == 1);
    assert(active_downloads.front() == ctrl);

    // make sure that this stupid move just gives us a pointer to
    // the already started download
    try 
    {
        ctrl = client->start_download(prog_table.at(2));
    } catch (libcow::exception& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    ctrl->unset_piece_finished_callback();

    for(size_t i = 11; i <= 15; ++i) {
 
        //ctrl->debug_print();

        size_t playback_pos = i*256*1024;
        std::cout << "setting playback position to " << playback_pos << std::endl;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(250);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(150);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(50);
        playback_pos += 64;
        ctrl->set_playback_position(playback_pos);
        libcow::system::sleep(10);
    }

    //client->remove_download(ctrl);

    // should give a warning in the log, but not crash
    //client->remove_download(ctrl);

    delete client;

    libcow::system::sleep(3000);

    return 0;
}
