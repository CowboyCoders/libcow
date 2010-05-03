#include "cow/libcow_def.hpp"
#include "cow/download_control_event_handler.hpp"
#include "cow/piece_request.hpp"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <libtorrent/alert_types.hpp>
#include <algorithm>

using namespace libcow;

static int bittorrent_source_id = 2;
static int disk_source_id = 1;

download_control_event_handler::download_control_event_handler(libtorrent::torrent_handle& h)
    : is_libtorrent_ready_(false),
      torrent_handle_(h)
{
    piece_origin_ = std::vector<int>(torrent_handle_.get_torrent_info().num_pieces(),0);
    disp_ = new dispatcher(0);
}

download_control_event_handler::~download_control_event_handler()
{
    delete disp_;
}

void download_control_event_handler::set_piece_src(int source, size_t piece_index)
{
    disp_->post(boost::bind(
        &download_control_event_handler::handle_set_piece_src, this, source, piece_index));
}

void download_control_event_handler::handle_set_piece_src(int source, size_t piece_index) 
{
    if(piece_index < piece_origin_.size() &&
        piece_origin_[piece_index] == 0)
    {
        piece_origin_[piece_index] = source;
    }
}

bool download_control_event_handler::current_state(std::vector<int>& state)
{
    boost::function<bool()> functor = 
        boost::bind(&download_control_event_handler::handle_current_state,
                    this,
                    boost::ref(state));
    boost::packaged_task<bool> task(functor);
    boost::unique_future<bool> future = task.get_future();

    disp_->post(boost::bind(&boost::packaged_task<bool>::operator(), 
                boost::ref(task)));
    std::cout << "waiting for future" << std::endl;
    future.wait();
    return future.get();
}

bool download_control_event_handler::handle_current_state(std::vector<int>& state)
{
    std::cout << "pieces_origin_.size() " << piece_origin_.size() << std::endl;
    std::vector<int>::const_iterator sources_iter;
    for(sources_iter = piece_origin_.begin();
        sources_iter != piece_origin_.end();
        ++sources_iter) 
    {
        state.push_back(*sources_iter);
    }
    std::cout << "returning from for loop" << std::endl;
    return true;
}

void download_control_event_handler::invoke_after_init(boost::function<void(void)> callback)
{
    disp_->post(
        boost::bind(&download_control_event_handler::handle_invoke_after_init, 
                    this, 
                    callback));
}

void download_control_event_handler::handle_invoke_after_init(boost::function<void(void)> callback)
{
    if(is_libtorrent_ready_) {
        callback();
    } else {
        startup_complete_callbacks_.push_back(callback);
    }
}

void download_control_event_handler::set_disk_source()
{
    if(torrent_handle_.is_valid()) {
        if(!torrent_handle_.is_seed()) {
            libtorrent::bitfield pieces = torrent_handle_.status().pieces;
            for(int i = 0; i < pieces.size(); ++i) {
                if(pieces[i]) {
                    piece_origin_[i] = disk_source_id;
                }
            }
        } else {
            std::fill(piece_origin_.begin(),piece_origin_.end(), disk_source_id);
        }
    }
}
void download_control_event_handler::signal_startup_callbacks()
{
    std::vector<boost::function<void()> >::iterator it;
    
    // we recieved that libtorrent is ready, 
    // hence all data is added from disk
    set_disk_source();

    for(it = startup_complete_callbacks_.begin(); it != startup_complete_callbacks_.end(); ++it) {
        (*it)();
    }
    startup_complete_callbacks_.clear();
}

void download_control_event_handler::signal_startup_complete()
{
    disp_->post(
        boost::bind(&download_control_event_handler::set_libtorrent_ready, this));
    disp_->post(
        boost::bind(&download_control_event_handler::signal_startup_callbacks,this));
}

void download_control_event_handler::signal_piece_finished(int piece_index)
{
    disp_->post(
        boost::bind(&download_control_event_handler::update_piece_requests, this, piece_index));
    int source;
    
    if(piece_index < piece_origin_.size()) {
        source = piece_origin_[piece_index];
    }

    if(source == 0) {
        // this piece originates from bittorrent since it hasn't been added by any other source
       source = bittorrent_source_id;
       piece_origin_[piece_index] = source;
    }
    
    disp_->post(
        boost::bind(&download_control_event_handler::invoke_piece_finished_callback, this, piece_index,source));
}

void download_control_event_handler::set_libtorrent_ready()
{
    is_libtorrent_ready_ = true;
}

void download_control_event_handler::invoke_when_downloaded(const std::vector<int>& pieces, 
                                              boost::function<void(std::vector<int>)> callback)
{
    invoke_after_init(
        boost::bind(&download_control_event_handler::handle_invoke_when_downloaded,
                    this,
                    pieces,
                    callback));

}

void download_control_event_handler::handle_invoke_when_downloaded(const std::vector<int>& pieces, 
                                                     boost::function<void(std::vector<int>)> callback)
{
    std::vector<int> missing = missing_pieces(pieces);
    if(missing.size() != 0) {
        std::list<int>* piece_list = new std::list<int>(missing.begin(),missing.end());
        std::vector<int>* org_pieces = new std::vector<int>(pieces);
        
        piece_request req(piece_list,callback,org_pieces);
        std::vector<libcow::piece_request> reqs;
        
        std::vector<int>::iterator it;
        
        for(it = missing.begin(); it != missing.end(); ++it) {
            int piece_id = *it;
            {
                piece_nr_to_request_.insert(std::pair<int,piece_request>(piece_id,req));
            }
            reqs.push_back(libcow::piece_request(
                torrent_handle_.get_torrent_info().piece_length(), piece_id, 1));
        }
    } else {
        callback(pieces);
    }
}

void download_control_event_handler::update_piece_requests(int piece_id)
{
    std::pair<std::multimap<int,piece_request>::iterator,
              std::multimap<int,piece_request>::iterator> bounds;
    bounds = piece_nr_to_request_.equal_range(piece_id);
    std::multimap<int,piece_request>::iterator it;
    
    for(it = bounds.first; it != bounds.second; ++it) {
        piece_request req = (*it).second;
        std::list<int>* pieces = req.pieces_;
        pieces->remove(piece_id);
        if(pieces->size() == 0) {
            std::vector<int> org_pieces = *(req.org_pieces_);
            piece_request::callback func = req.callback_;
            delete pieces;
            delete req.org_pieces_;
            func(org_pieces);
        } 
    }
    if(piece_nr_to_request_.find(piece_id) != piece_nr_to_request_.end()) {
        piece_nr_to_request_.erase(piece_id);
    }
}

std::vector<int> download_control_event_handler::missing_pieces(const std::vector<int>& pieces)
{
    libtorrent::torrent_status status = torrent_handle_.status();
    std::vector<int> missing;
    
    if(status.state == libtorrent::torrent_status::seeding) {
        return missing;
    }
	else if (status.state == libtorrent::torrent_status::downloading ||
             status.state == libtorrent::torrent_status::finished)
    {
	    const libtorrent::bitfield& piece_status = status.pieces;
        std::vector<int>::const_iterator it;
		for (it = pieces.begin(); it != pieces.end(); ++it) {
			int piece_idx = *it;
            if (!piece_status[piece_idx]) {
				missing.push_back(piece_idx);
			}
		}
        return missing;
	} else {
        return pieces;
    }
}

void download_control_event_handler::invoke_piece_finished_callback(int piece_index,int device)
{
    if(!piece_finished_callback_.empty()) {
        piece_finished_callback_(piece_index,device);
    }
}

void download_control_event_handler::set_piece_finished_callback(const boost::function<void(int,int)>& func)
{
    disp_->post(boost::bind(
        &download_control_event_handler::handle_set_piece_finished_callback, this, func));
}

void download_control_event_handler::handle_set_piece_finished_callback(const boost::function<void(int,int)>& func)
{
    piece_finished_callback_ = func;
}

void download_control_event_handler::unset_piece_finished_callback()
{
    disp_->post(boost::bind(
        &download_control_event_handler::handle_unset_piece_finished_callback, this));
}

void download_control_event_handler::handle_unset_piece_finished_callback()
{
    piece_finished_callback_.clear();
}
