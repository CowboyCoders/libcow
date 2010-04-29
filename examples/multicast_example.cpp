#include <cow/cow.hpp>
int main() {
    libcow::cow_client example_client;
    example_client.start_logger();

    std::string download_dir = ".";
    example_client.set_download_directory(download_dir); 

    int bt_port = 23454; 
    example_client.set_bittorrent_port(bt_port);

    libcow::program_table table;
    std::string server_url = "http://cowboycoders.se/program_table.xml";
    table.load_from_http(server_url);
    libcow::program_info program_info = table.at(0);
   
    example_client.register_download_device_factory(
        boost::shared_ptr<libcow::download_device_factory>(
            new libcow::multicast_server_connection_factory()), 
        "multicast");

    example_client.start_download(program_info);

    std::cout << "Running multicast_example, press ENTER to quit...";
    char q;
    do {
        std::cin.get(q);
    }
    while(q != '\n');
    std::cout << "Quitting..." << std::endl;
    return 0;
}
