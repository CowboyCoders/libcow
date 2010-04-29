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

#include <cow/cow.hpp>

int main() {
    // Create a cow_client.
    libcow::cow_client basic_example_client;
    // Start logging.
    basic_example_client.start_logger();

    // Set download directory for the movie.
    std::string download_dir = ".";
    basic_example_client.set_download_directory(download_dir); 

    // Set the port to bittorrent.
    int bt_port = 23454; 
    basic_example_client.set_bittorrent_port(bt_port);

    // A properties map for setting to pass to download_device.
    libcow::properties properties;
    // URL to the torrent file.
    properties["torrent"] = "http://cowboycoders.se/big_buck_bunny.mpg.torrent";

    // Create a device_map to handle different downloading sources.
    libcow::device_map device_map;
    // Add torrent as a downloading source.
    device_map["torrent"] = properties;

    // Create a program_info struct and fill it with necessary data.
    libcow::program_info program_info;
    program_info.id = 1;
    program_info.name = "Big Buck Bunny";
    program_info.description = "'Big' Buck wakes up in his rabbit hole...";
    program_info.download_devices = device_map;

    // Start downloading and save the pointer (later needed to stop the download).
    libcow::download_control* controller = basic_example_client.start_download(program_info);
    std::cout << "Running basic_example, press ENTER to quit...";
    char q;
    do {
        std::cin.get(q);
    }
    while(q != '\n');
    std::cout << "Quitting..." << std::endl;
    
    // Stop download.
    basic_example_client.remove_download(controller);
    // Stop logging.
    basic_example_client.stop_logger();
    // Quit :-)
    return 0;
}
