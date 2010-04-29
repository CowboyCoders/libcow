#include <cow/cow.hpp>
int main() {
    libcow::cow_client basic_example_client;
    basic_example_client.start_logger();

    std::string download_dir = ".";
    basic_example_client.set_download_directory(download_dir); 

    int bt_port = 23454; 
    basic_example_client.set_bittorrent_port(bt_port);

    libcow::program_table table;
    std::string server_url = "http://cowboycoders.se/program_table.xml";
    table.load_from_http(server_url);
    libcow::program_info program_info = table.at(0);

    basic_example_client.start_download(program_info);
    std::cout << "Running program_table_example, press ENTER to quit...";
    char q;
    do {
        std::cin.get(q);
    }
    while(q != '\n');
    std::cout << "Quitting..." << std::endl;
    return 0;
}
