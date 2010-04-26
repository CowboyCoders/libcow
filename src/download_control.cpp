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

#include "cow/libcow_def.hpp"
#include "cow/download_control.hpp"
#include "cow/piece_data.hpp"
#include "cow/piece_request.hpp"
#include "cow/progress_info.hpp"
#include "cow/download_device.hpp"
#include "cow/system.hpp"

#include <boost/log/trivial.hpp>
#include <libtorrent/peer_info.hpp>

#include <iostream>
#include <time.h>
#include <cstdlib>

using namespace libcow;

download_control::download_control(const libtorrent::torrent_handle& handle, 
                                   int critical_window_length,
                                   int critical_window_timeout)
    : piece_sources_(0),
    handle_(handle),
    disp_(critical_window_timeout), //TODO: 
    critical_window_(critical_window_length)

{
    srand (time(NULL));
    piece_origin_ = std::vector<int>(handle_.get_torrent_info().num_pieces(),0);
    critically_requested_ = std::vector<bool>(handle_.get_torrent_info().num_pieces(), false);
}

download_control::~download_control()
{
    // must destruct download_devices BEFORE the torrent handle falls out of scope!
    download_devices_.clear();
}

progress_info download_control::get_progress()
{
    libtorrent::torrent_status status = handle_.status();

    if(status.state == libtorrent::torrent_status::downloading) 
    {
        return progress_info(progress_info::downloading, status.progress, status.pieces, piece_origin_);
    }
    // return an all-true bitfield if we're seeding
    else if(status.state == libtorrent::torrent_status::seeding) 
    {
        libtorrent::bitfield all_true(num_pieces(), true);
        return progress_info(progress_info::seeding, status.progress, all_true, piece_origin_);
    }
    // return an all-false bitfield in all other cases
    else
    {
        progress_info::torrent_state state;
        switch(status.state)
        {
        case libtorrent::torrent_status::allocating: 
            state = progress_info::allocating; 
            break;
        case libtorrent::torrent_status::checking_files: 
            state = progress_info::checking_files; 
            break;
        case libtorrent::torrent_status::checking_resume_data: 
            state = progress_info::checking_resume_data; 
            break;
        case libtorrent::torrent_status::downloading_metadata: 
            state = progress_info::downloading_metadata; 
            break;
        case libtorrent::torrent_status::finished:
            state = progress_info::finished; 
            break;
        case libtorrent::torrent_status::queued_for_checking:
            state = progress_info::queued_for_checking; 
            break;
        default:
            state = progress_info::unknown;
        }
    	
        libtorrent::bitfield all_false(num_pieces(), false);
        return progress_info(state, status.progress, all_false, piece_origin_);
    }
}

int download_control::piece_length()
{
	libtorrent::torrent_info ti = handle_.get_torrent_info();
	return ti.piece_length();
}

void download_control::add_pieces(int id, const std::vector<piece_data>& pieces)
{
    libtorrent::torrent_info info = handle_.get_torrent_info();
    std::vector<piece_data>::const_iterator iter;

    for(iter = pieces.begin(); iter != pieces.end(); ++iter) 
    {
        if((int)iter->data.size() < info.piece_size(iter->index)) {
            BOOST_LOG_TRIVIAL(warning) << "download_control::add_pieces: "
                << "trying to add piece with index " << iter->index
                << " and size " << iter->data.size() << ", but expected size is "
                << info.piece_size(iter->index);
            continue;
        }
        disp_.post(boost::bind(&download_control::set_piece_src, this, id, iter->index));

        BOOST_LOG_TRIVIAL(debug) << "download_control::add_piece: adding piece with index "
                                 << iter->index;
        handle_.add_piece(iter->index, iter->data.data());
    }
}

void download_control::add_download_device(download_device* dd)
{
    boost::shared_ptr<download_device> dd_ptr;
    dd_ptr.reset(dd);
    download_devices_.push_back(dd_ptr);
}

bool download_control::has_data(size_t offset, size_t length)
{
    assert(length > 0);
	
    const libtorrent::torrent_info& info = handle_.get_torrent_info();
	int piece_size = info.piece_length();
    
	size_t piece_start = offset / piece_size;
	size_t piece_end = (offset + length - 1) / piece_size;

    //std::cout << "HASDATA SAYS: piece_start: " << piece_start
    //          << " piece_end: " << piece_end << std::endl;

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

size_t download_control::read_data(size_t offset, libcow::utils::buffer& buffer)
{
	while(!has_data(offset, buffer.size())){
        libcow::system::sleep(10);
    }

    if(!file_handle_.is_open()) {
        // FIXME: reads from file with index 0 ONLY!
        libtorrent::file_entry file_entry = handle_.get_torrent_info().files().at(0);
    std::string filename = file_entry.path;

#if 0
        std::cout << "filename: " << filename << std::endl;
        std::cout << "offset: " << offset << " bufsize: " << buffer.size() << std::endl;
#endif

        file_handle_.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
        if(!file_handle_.is_open()) {
            BOOST_LOG_TRIVIAL(warning) << "Could not open file: " << filename;
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

void download_control::pre_buffer(size_t offset, size_t length)
{

}

void download_control::pre_buffer(const std::vector<libcow::piece_request> requests)
{
    std::vector<boost::shared_ptr<download_device> >::iterator it;

    download_device* random_access_device = 0;

    for(it = download_devices_.begin(); it != download_devices_.end(); ++it) {
        download_device* current = it->get();
        if(current != 0 && current->is_random_access()) {
            random_access_device = current;
            break;
        }
    }

    if(random_access_device) 
    {
        random_access_device->get_pieces(requests);
    } else {
        BOOST_LOG_TRIVIAL(info) << "Can't pre_buffer. No random access device available.";
    }
}

size_t download_control::file_size() const
{
    const libtorrent::file_entry& file_entry = handle_.get_torrent_info().files().at(0);
    return file_entry.size;
}

void download_control::debug_print()
{
    progress_info info = get_progress();
    libtorrent::bitfield progress = info.downloaded();

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

void download_control::set_playback_position(size_t offset)
{
    // don't fiddle with download strategies if seeding
    if(handle_.status().state == libtorrent::torrent_status::seeding) {
        return;
    }

    assert(offset >= 0);

    // create a vector of pieces to prioritize
    int first_piece_to_prioritize = offset / piece_length();

    std::cout << "first_piece_to_prioritize: " << first_piece_to_prioritize << std::endl;

    // set low priority for pieces before playback position
    for(int i = 0; i < first_piece_to_prioritize; ++i) {
        handle_.piece_priority(i, 1);
    }

    // set high priority for pieces in critical window
    for(int i = 0; i < critical_window(); ++i) {
        int idx = first_piece_to_prioritize + i;
        if(idx >= handle_.get_torrent_info().num_pieces()) break;
        handle_.piece_priority(idx, 7);
    }

    // get an up to date list of random access devices
    std::vector<download_device*> random_access_devices;

    std::vector<boost::shared_ptr<download_device> >::iterator it;

    for(it = download_devices_.begin(); it != download_devices_.end(); ++it)
    {
        download_device* dev = it->get();
        if(dev && dev->is_random_access()) {
            random_access_devices.push_back(dev);
        }
    }
    
    if(random_access_devices.size() > 0) {
        // load balancing
        int device_index = rand() % random_access_devices.size();

        download_device* dev = random_access_devices[device_index];
        disp_.post_delayed(
            boost::bind(&download_control::fetch_missing_pieces, this, 
                        dev, 
                        first_piece_to_prioritize, 
                        first_piece_to_prioritize + critical_window() - 1, 
                        _1));
    }
}

void download_control::fetch_missing_pieces(download_device* dev, 
                                            int first_piece, 
                                            int last_piece, 
                                            boost::system::error_code& error)
{
    assert(last_piece >= first_piece);
    
    libtorrent::torrent_status status = handle_.status();
    libtorrent::bitfield pieces = status.pieces;

    assert(last_piece < pieces.size());

    std::vector<libcow::piece_request> reqs;

    for(int i = first_piece; i <= last_piece; ++i) {
        // don't request non-existing pieces
        if(i >= num_pieces()) {
            break;
        }
        // request only pieces that we don't already have or 
        // haven't alread requested.
        if(!pieces[i] && !critically_requested_[i]) {
            reqs.push_back(libcow::piece_request(piece_length(), i, 1));
            critically_requested_[i] = true;
            BOOST_LOG_TRIVIAL(debug) 
                << "Falling back to random access download devices for piece " << i;
        }
    }

    if(reqs.size() > 0) {
        dev->get_pieces(reqs);
    }
}
