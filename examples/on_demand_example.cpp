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

    // URL to the XML file and the timeout for the the connection (in seconds).
    std::string server_url = "http://cowboycoders.se/program_table.xml";
    size_t timeout = 120;
    // Download and parse the XML. Creates a table of all available programs.
    table.load_from_http(server_url, timeout);
    // Retrieve the info for program with ID 0, which is big_buck_bunny.mpg.
    libcow::program_info program_info = table.at(0);
   
    // Registers a new on_demand_server_connection_factory. This enables the libcow::cow_client
    // to be able to dynamically create libcow::on_demand_server_connections that we can request
    // data pieces from. The name "http" in the function below must correspond to a download_device
    // in the XML file.
    example_client.register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::on_demand_server_connection_factory()), 
        "http");

    // Start downloading the movie. A libcow::download_control pointer is created
    // that can be used to extract information about the download, as well as stop it.
    libcow::download_control* controller = example_client.start_download(program_info);
    
    // In order to request data from the on_demand_server, we must first create a vector of piece_requests.
    std::vector<libcow::piece_request> requests;
    // The index of the piece to request.
    size_t index = 0;
    // The number of pieces in a row to request. In this example, we will request pieces 0 to 9.
    size_t number_of_pieces = 10;
    // Create a new libcow::piece_request and add it to the vector.
    requests.push_back(libcow::piece_request(controller->piece_length(), index, number_of_pieces));
    // Sends the requests to the libcow::download_control. After some time, the
    // pieces will be visible in the log.
    controller->pre_buffer(requests);




    std::cout << "Running on_demand_example, press ENTER to quit...";
    char q;
    do {
        std::cin.get(q);
    }
    while(q != '\n');
    std::cout << "Quitting..." << std::endl;
    
    // Stop the download.
    example_client.remove_download(controller);
    // Quit :-)
    return 0;
}
