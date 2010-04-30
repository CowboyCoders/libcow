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
    libcow::cow_client example_client;
    // Start logging. All logging will be sent to a file called program_table_example.log.
    example_client.start_logger();

    // Set download directory for the movie.
    std::string download_dir = ".";
    example_client.set_download_directory(download_dir); 

    // Set the port to use when downloading using BitTorrent.
    int bt_port = 23454; 
    example_client.set_bittorrent_port(bt_port);

    // Create a libcow::program_table object that will be able to parse an XML
    // containing info about one or more programs. In this example, the XML file
    // will be fetched from a web server.
    libcow::program_table table;

    // URL to the XML file and timeout for the connection (in seconds).
    std::string server_url = "http://cowboycoders.se/program_table.xml";
    size_t timeout = 120;
    // Download and parse the XML. Creates a table of all available programs.
    table.load_from_http(server_url, timeout);
    // Retrieve the info for program with ID 0, which is big_buck_bunny.mpg.
    libcow::program_info program_info = table.at(0);

    // Start downloading the movie. A libcow::download_control pointer is created
    // that can be used to extract information about the download, as well as stop it.
    libcow::download_control* controller = example_client.start_download(program_info);
    
    
    
    
    std::cout << "Running program_table_example, press ENTER to quit...";
    char q;
    do {
        std::cin.get(q);
    }
    while(q != '\n');
    std::cout << "Quitting..." << std::endl;
    
    // Stop the download.
    example_client.remove_download(controller);
    // Stop the logging.
    example_client.stop_logger();
    // Quit :-)
    return 0;
}
