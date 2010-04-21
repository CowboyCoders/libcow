#ifndef __download_device_description__
#define __download_device_description__

namespace libcow {

    struct download_device_description 
	{
        std::string name;
        std::map<std::string, std::string> properties;
    };
}

#endif

