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
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of CowboyCoders.
*/

#ifndef ___libcow_download_control___
#define ___libcow_download_control___

#include "buffer.hpp"
#include "dispatcher.hpp"

#include <boost/noncopyable.hpp>
#include <boost/log/trivial.hpp>
#include <libtorrent/torrent_handle.hpp>

namespace libcow {

    class LIBCOW_EXPORT download_control : public boost::noncopyable
    {
    public:
        download_control(const libtorrent::torrent_handle& handle);
        ~download_control();

        progress_info get_progress();
        void pre_buffer(size_t offset, size_t length);
        void pre_buffer(const std::vector<libcow::piece_request> requests);
        size_t read_data(size_t offset, libcow::utils::buffer& buffer);
        bool has_data(size_t offset, size_t length);

		int piece_length();
		
        void add_pieces(int id, const std::vector<libcow::piece_data>& pieces);

        void set_piece_src(int source, int piece_index) {
            BOOST_LOG_TRIVIAL(info) << "Setting source " << source << " for piece id " << piece_index;
            piece_origin_[piece_index] = source;
        }

        void add_download_device(download_device* dd);

        std::map<int,std::string> * piece_sources_;
        std::vector<int> piece_origin_;

    private:
        libtorrent::torrent_handle handle_;

        std::vector<boost::shared_ptr<download_device> > download_devices;

        std::ifstream file_handle_;

        dispatcher disp_;

        // cow_client should have access to the torrent_handle
        friend class libcow::cow_client;

    };
}

#endif // ___libcow_download_control___
