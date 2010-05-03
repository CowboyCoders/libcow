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

#include "cow/libcow_def.hpp"
#include "cow/download_control.hpp"
#include "cow/piece_data.hpp"
#include "cow/piece_request.hpp"
#include "cow/progress_info.hpp"
#include "cow/download_device.hpp"
#include "cow/system.hpp"
#include "cow/dispatcher.hpp"

#include <boost/log/trivial.hpp>
#include <libtorrent/peer_info.hpp>

#include <iostream>
#include <time.h>
#include <cstdlib>
#include <map>
#include <cassert>

using namespace libcow;

download_control::download_control(const libtorrent::torrent_handle& handle, 
                                   int critical_window_length,
                                   int critical_window_timeout,
                                   int id)
    : handle_(handle),
      id_(id)

{
    srand (time(NULL));

    event_handler_ = new download_control_event_handler(handle_);
    worker_ = new download_control_worker(handle_, critical_window_length, critical_window_timeout);
}

download_control::~download_control()
{
    delete event_handler_;
    delete worker_;
}

int download_control::piece_length()
{
	libtorrent::torrent_info ti = handle_.get_torrent_info();
	return ti.piece_length();
}

/* NOTE: this function is safe to call from different threads
 * since it only calls libtorrent and the event dispatcher 
 */
//FIXME: queue add_piece calls if libtorrent is still checking hash
void download_control::add_pieces(int id, const std::vector<piece_data>& pieces)
{
    if(!handle_.is_seed()) {
    libtorrent::torrent_info info = handle_.get_torrent_info();
    std::vector<piece_data>::const_iterator iter;

        libtorrent::torrent_status status = handle_.status();
        for(iter = pieces.begin(); iter != pieces.end(); ++iter) 
        {
            if((int)iter->data.size() < info.piece_size(iter->index)) {
                BOOST_LOG_TRIVIAL(warning) << "download_control::add_pieces: "
                    << "trying to add piece with index " << iter->index
                    << " and size " << iter->data.size() << ", but expected size is "
                    << info.piece_size(iter->index);
                continue;
            }
            
            const libtorrent::bitfield& pieces = status.pieces;
            if(pieces[iter->index]) {
                continue;
            }
            
            set_piece_src(id, iter->index);

            BOOST_LOG_TRIVIAL(debug) << "download_control::add_piece: adding piece with index "
                                     << iter->index;
            handle_.add_piece(iter->index, iter->data.data());
        }
    }
}

void download_control::add_download_device(download_device* dd)
{
    worker_->add_download_device(dd);
}

bool download_control::has_data(size_t offset, size_t length)
{
    assert(length > 0);
	
    const libtorrent::torrent_info& info = handle_.get_torrent_info();
	int piece_size = info.piece_length();
    
	size_t piece_start = offset / piece_size;
	size_t piece_end = (offset + length - 1) / piece_size;

    if ((int)piece_end >= info.num_pieces() || (int)piece_start >= info.num_pieces()){
        throw std::out_of_range("has_data: piece index out of range");
	}

    libtorrent::torrent_status status = handle_.status();
    
    if(status.state == libtorrent::torrent_status::seeding)
    {
        return true;
    }
	else if (status.state == libtorrent::torrent_status::downloading ||
             status.state == libtorrent::torrent_status::finished)
    {
	    const libtorrent::bitfield& pieces = status.pieces;
		for (size_t i = piece_start; i <= piece_end ; ++i) {
			if (!pieces[i]) {
				return false;
			}
		}
        return true;
	}
    else
    {
        return false;
    }
}

size_t download_control::bytes_available(size_t offset) const
{
    const libtorrent::torrent_info& info = handle_.get_torrent_info();
	int piece_size = info.piece_length();
    
	size_t piece_start = offset / piece_size;

    if ((int)piece_start >= info.num_pieces()) {
        throw std::out_of_range("bytes_available: offset out of range");
	}

    libtorrent::torrent_status status = handle_.status();
	if (status.state != libtorrent::torrent_status::seeding) {
	    const libtorrent::bitfield& pieces = status.pieces;

        if (!pieces[piece_start]) {
            return 0;
        }

		for (int i = piece_start + 1; i < info.num_pieces(); ++i) {
			if (!pieces[i]) {
				return i * piece_size - offset;
			}
		}
	}

    return info.file_at(0).size - offset;
}


std::string download_control::filename() const
{
    assert(handle_.get_torrent_info().files().num_files() == 1);
    
    libtorrent::file_entry file_entry = handle_.get_torrent_info().files().at(0);
    return file_entry.path;
}

size_t download_control::read_data(size_t offset, libcow::utils::buffer& buffer)
{
	while(!has_data(offset, buffer.size())){
        libcow::system::sleep(10);
    }

    if(!file_handle_.is_open()) {
        std::string file_name = filename();
#if 0
        std::cout << "filename: " << filename << std::endl;
        std::cout << "offset: " << offset << " bufsize: " << buffer.size() << std::endl;
#endif

        file_handle_.open(file_name.c_str(), std::ios_base::in | std::ios_base::binary);
        if(!file_handle_.is_open()) {
            BOOST_LOG_TRIVIAL(warning) << "Could not open file: " << file_name;
            return 0;
        }
#if 0
        std::cout << "READ DATA!" << std::endl;
        std::cout << std::endl;
#endif
    }

    file_handle_.seekg (0, std::ios::end);
    size_t length = file_handle_.tellg();
    file_handle_.seekg(offset, std::ios::beg);

    size_t bytes_to_read = std::min(length-offset, buffer.size());
    file_handle_.read(buffer.data(), bytes_to_read);
    
    return static_cast<size_t>(file_handle_.gcount());
}

    std::vector<libcow::piece_request>::const_iterator piece_iter;
    for(piece_iter = requests.begin(); piece_iter != requests.end(); ++piece_iter) {
        if(handle_.is_valid()) {
            handle_.piece_priority((*piece_iter).index,7);
        }
         

size_t download_control::file_size() const
{
    const libtorrent::file_entry& file_entry = handle_.get_torrent_info().files().at(0);
    return file_entry.size;
}

void download_control::debug_print()
{
    libtorrent::bitfield progress = handle_.status().pieces;

    std::cout << "num_pieces: " << num_pieces() << std::endl;
    std::cout << "progress.size: " << progress.size() << std::endl;

    libtorrent::bitfield::const_iterator bitfield_iter;

    std::cout << "--- PROGRESS ---" << std::endl;
    for(bitfield_iter = progress.begin(); bitfield_iter != progress.end(); ++bitfield_iter) {
        std::cout << *bitfield_iter;
    }
    std::cout << std::endl;

    std::vector<libtorrent::peer_info> peers;
    handle_.get_peer_info(peers);
    std::vector<libtorrent::peer_info>::iterator peer_iter;
    std::cout << "interesting peers: ";
    int interesting_peer_count = 0;
    for(peer_iter = peers.begin(); peer_iter != peers.end(); ++peer_iter) {
        if(peer_iter->interesting) {
            std::cout << peer_iter->ip << " ";
            ++interesting_peer_count;
        }
    }
    if(interesting_peer_count == 0) {
        std::cout << "no interesting peers";
    }
    std::cout << std::endl;

    libtorrent::torrent_status status = handle_.status();
    std::cout << "download rate: "  
              << status.download_rate / 1048576.0 << " MiB/s"
              << " | upload rate: " 
              << status.upload_rate / 1048576.0 << " MiB/s"
			  << " | seeds: " << status.list_seeds
		 	  << " | peers: " << status.list_peers << std::endl;

}

    if(handle_.is_valid()) {
        libtorrent::torrent_status status = handle_.status();
        if(status.state == libtorrent::torrent_status::downloading ||
           status.state == libtorrent::torrent_status::finished ||
           status.state == libtorrent::torrent_status::seeding) 
            return true;
        }
        // we might get the event here via callback, need to check 
        // the bool variable
        
    }
    return false;
}

void download_control::wait_for_startup(boost::function<void(void)> callback)
{
    if(is_running()) {