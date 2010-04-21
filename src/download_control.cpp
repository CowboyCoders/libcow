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

#include <boost/log/trivial.hpp>

#include <iostream>

using namespace libcow;

download_control::download_control(const libtorrent::torrent_handle& handle)
    : handle_(handle) 
{
}

download_control::~download_control()
{
}

progress_info download_control::get_progress()
{
	static int has_shit = 0;
    libtorrent::torrent_status status = handle_.status();

    std::cout << "download rate: "  
              << status.download_rate / 1048576.0 << " MiB/s"
              << " | upload rate: " 
              << status.upload_rate / 1048576.0 << " MiB/s"
			  << " | seeds: " << status.list_seeds
		 	  << " | peers: " << status.list_peers << std::endl;

    progress_info::torrent_state state;
    switch (status.state) {
    case libtorrent::torrent_status::allocating: 
        state = progress_info::allocating; 
        break;
    case libtorrent::torrent_status::checking_files: 
        state = progress_info::checking_files; 
        break;
    case libtorrent::torrent_status::checking_resume_data: 
        state = progress_info::checking_resume_data; 
        break;
    case libtorrent::torrent_status::downloading: 
        state = progress_info::downloading;
		if(!has_shit) {
			std::cout << "on demand opened? " << download_devices[0].get()->is_open() << std::endl;    
			std::vector<libcow::piece_request> gief_movie;
			gief_movie.push_back(piece_request(256*1024, 0, 30));
			download_devices[0].get()->get_pieces(gief_movie);
			has_shit = 1;
		}
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
    case libtorrent::torrent_status::seeding:
        state = progress_info::seeding; 
        break;
    default:
        state = progress_info::unknown;
    }
	
	std::cout << "has_data: " << has_data(0,1000000) << std::endl;

    std::vector<libtorrent::peer_info> peers;
    handle_.get_peer_info(peers);
    std::vector<libtorrent::peer_info>::iterator iter;
    std::cout << "interesting? ";
    for(iter = peers.begin(); iter != peers.end(); ++iter) {
        std::cout << iter->ip << " " << iter->interesting << "; ";   
    }
    std::cout << std::endl;

    return progress_info(state, status.progress);
}

int download_control::piece_length()
{
	libtorrent::torrent_info ti = handle_.get_torrent_info();
	return ti.piece_length();
}

void download_control::add_pieces(const std::vector<piece_data>& pieces)
{
    libtorrent::torrent_info info = handle_.get_torrent_info();
    std::vector<piece_data>::const_iterator iter;
    for(iter = pieces.begin(); iter != pieces.end(); ++iter) 
    {
        if(iter->data.size() != info.piece_size(iter->index)) {
            BOOST_LOG_TRIVIAL(warning) << "download_control::add_pieces: "
                << "trying to add piece with index " << iter->index
                << "and size " << iter->data.size() << ", but expected size is "
                << info.piece_size(iter->index);
            return; //TODO: throw exception here?
        }

        handle_.add_piece(iter->index, iter->data.data());
    }
}

void download_control::add_download_device(download_device* dd)
{
    boost::shared_ptr<download_device> dd_ptr;
    dd_ptr.reset(dd);
    download_devices.push_back(dd_ptr);
}

bool download_control::has_data(size_t offset, size_t length)
{
	bool seeding = (handle_.status().state == libtorrent::torrent_status::seeding);
	
	libtorrent::bitfield pieces = handle_.status().pieces;
	int piece_size = handle_.get_torrent_info().piece_length();
	assert(piece_size >= 0);
	size_t piece_start = offset / piece_size;
	size_t piece_end = (offset + length) / piece_size;
	if (piece_end >= handle_.status().num_pieces || piece_start >= handle_.status().num_pieces){
		return false;
	}
	if (!seeding){
		/*
		// piece debugging info broet-output
		std::cout<<"piece_start: " << piece_start << " piece_end: " << piece_end << std::endl;
		for (int i = 0 ; i < pieces.size(); i++){
			std::cout<< pieces[i];
		}
		std::cout<<std::endl;
		*/
		for (; piece_start <= piece_end ; piece_start++){
			if (!pieces[piece_start]){
				return false;
			}
		}
	}
    return true;
}

size_t download_control::read_data(size_t offset, libcow::utils::buffer& buffer){
	if (has_data(offset, buffer.size())){
		int piece_size = handle_.get_torrent_info().piece_length();
		int start_piece = offset / piece_size;
		int offset_in_start_piece = (offset - start_piece * piece_size);
		char* data = buffer.data();
		
		int bytes_left = buffer.size();

		int piece_to_read = start_piece;
		int offset_in_piece = offset_in_start_piece;
		while(bytes_left > 0){
			std::cout << "Reading data from piece nr " << piece_to_read << ", index " << offset_in_piece << std::endl;
			size_t data_read = handle_.get_storage_impl()->read(data, piece_to_read,
				offset_in_piece, std::min(bytes_left, piece_size - offset_in_piece));
			std::cout << "Read " << data_read << " bytes of data." << std::endl;
			bytes_left -= data_read;
			if(data_read == 0)
				break;
			piece_to_read++;
			offset_in_piece = 0;
		}
		return buffer.size()-bytes_left;
	}
	return 0;
}