#ifndef ___libcow_cow_client_worker___
#define ___libcow_cow_client_worker___

#include "cow/dispatcher.hpp"
#include <libtorrent/alert.hpp>
#include <libtorrent/torrent_handle.hpp>

namespace libcow 
{

    class cow_client_worker
    {
    public:
        cow_client_worker();
        ~cow_client_worker();
    };

}

#endif // ___libcow_cow_client_worker___