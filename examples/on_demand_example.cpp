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
            new libcow::on_demand_server_connection_factory()), 
        "http");

    libcow::download_control* controller = example_client.start_download(program_info);
    std::vector<libcow::piece_request> requests;
    size_t index = 0;
    size_t number_of_pieces = 10;
    requests.push_back(libcow::piece_request(controller->piece_length(), index, number_of_pieces));
    controller->pre_buffer(requests);

    std::cout << "Running on_demand_example, press ENTER to quit...";
    char q;
    do {
        std::cin.get(q);
    }
    while(q != '\n');
    std::cout << "Quitting..." << std::endl;
    return 0;
}
