#include <cow/cow.hpp>

int main() {
    libcow::cow_client basic_example_client;
    basic_example_client.start_logger();

    std::string download_dir = ".";
    basic_example_client.set_download_directory(download_dir); 

    int bt_port = 23454; 
    basic_example_client.set_bittorrent_port(bt_port);

    libcow::properties properties;
    properties["torrent"] = "http://cowboycoders.se/big_buck_bunny.mpg.torrent";

    libcow::device_map device_map;
    device_map["torrent"] = properties;

    libcow::program_info program_info;
    program_info.id = 1;
    program_info.name = "Big Buck Bunny";
    program_info.description = "'Big' Buck wakes up in his rabbit hole...";
    program_info.download_devices = device_map;

    basic_example_client.start_download(program_info);

    return 0;
}
