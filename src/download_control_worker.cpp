#include "cow/libcow_def.hpp"
#include "cow/download_control_worker.hpp"
#include "cow/download_device.hpp"
#include "cow/piece_request.hpp"
#include "cow/dispatcher.hpp"
#include "cow/utils/chunk.hpp"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <cmath>

using namespace libcow;

unsigned int download_control_worker::buffering_state_length_ = 10;

download_control_worker::download_control_worker(libtorrent::torrent_handle& h,
                                                 size_t critical_window_length,
                                                 int critical_window_timeout)
    : critical_window_(critical_window_length),
      torrent_handle_(h),
      buffering_state_counter_(0)
{
    critically_requested_ = 
        std::vector<bool>(torrent_handle_.get_torrent_info().num_pieces(), false);
    disp_ = new dispatcher(critical_window_timeout);

    is_running_ = true;
}

download_control_worker::~download_control_worker()
{
    delete disp_;
    download_devices_.clear();
}

void download_control_worker::set_critical_window(size_t length)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_set_critical_window, this, length));
}

void download_control_worker::handle_set_critical_window(size_t length)
{
    critical_window_ = length;
}

void download_control_worker::set_critical_window_timeout(int timeout)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_set_critical_window_timeout,this,timeout));
}

void download_control_worker::handle_set_critical_window_timeout(int timeout)
{
    // DJ HENRY SPIN THAT SHIT
}
        
void download_control_worker::set_piece_requested(int piece_index, bool req)
{
    disp_->post(boost::bind(&download_control_worker::handle_set_piece_requested,
                            this,
                            piece_index,
                            req));
}

void download_control_worker::handle_set_piece_requested(int piece_index, bool req)
{
    critically_requested_[piece_index] = req;
}

void download_control_worker::add_download_device(download_device* dd)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_add_download_device, this, dd));
}

void download_control_worker::handle_add_download_device(download_device* dd)
{
    boost::shared_ptr<download_device> dd_ptr;
    dd_ptr.reset(dd);
    download_devices_.push_back(dd_ptr);
}

void download_control_worker::pre_buffer(const chunk& c)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_download_strategy, this, c, true, true));

}

void download_control_worker::set_playback_position(size_t offset, bool force_request)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_set_playback_position, this, offset, force_request));
}

void download_control_worker::handle_set_playback_position(size_t offset, bool force_request)
{
    handle_download_strategy(chunk(offset, critical_window_), force_request, false);
}
        
std::map<int,std::string> download_control_worker::get_device_names()
{
    boost::function<std::map<int,std::string>()> functor =
        boost::bind(&download_control_worker::handle_get_device_names,
                    this);
    boost::packaged_task<std::map<int,std::string> > task(functor);
    boost::unique_future<std::map<int,std::string> > future = task.get_future();

    disp_->post(boost::bind(&boost::packaged_task<std::map<int,std::string> >::operator(),
                            boost::ref(task))); 
    future.wait();
    return future.get();

}

std::map<int,std::string> download_control_worker::handle_get_device_names()
{
    std::map<int,std::string> devices;
    devices[0] = "missing";
    devices[1] = "hdd";
    devices[2] = "bittorrent";
    
    std::vector<boost::shared_ptr<download_device> >::iterator it;
    for(it = download_devices_.begin(); it != download_devices_.end(); ++it) {
        download_device* dd = it->get();
        devices[dd->id()] = dd->type();
    }

    return devices;
}

void download_control_worker::handle_download_strategy(const chunk& c, bool force_request, bool pre_buffer)
{
    // don't fiddle with download strategies if seeding
    if(torrent_handle_.status().state == libtorrent::torrent_status::seeding) {
        return;
    }

    assert(c.offset() >= 0);

    // create a vector of pieces to
    const libtorrent::torrent_info& torrent_info = torrent_handle_.get_torrent_info();
    int first_piece_to_prioritize = c.offset() / torrent_info.piece_length();
    // "greedy" mapping from bytes to pieces. 
    // the total size (in bytes) of the priorizied pieces will at least be 'length'.

    int num_pieces_in_critical_window = 
        static_cast<int>(floor((c.offset() + c.length()) * 1.0 / torrent_info.piece_length()))
            - first_piece_to_prioritize + 1;
    
    //std::cout << "first_piece_to_prioritize: " << first_piece_to_prioritize << std::endl;

    if(buffering_state_counter_ > buffering_state_length_ && !pre_buffer) {
        // set low priority for pieces before playback position unless we're buffering
        for(int i = 0; i < first_piece_to_prioritize; ++i) {
            torrent_handle_.piece_priority(i, 1);
        }
    }

    int counter = 0;
    // set highest priority for the most critical pieces
    for(int i = 0; i < 2*num_pieces_in_critical_window; ++i, ++counter) {
        int idx = first_piece_to_prioritize + counter;
        if(idx >= torrent_info.num_pieces()) break;
        torrent_handle_.piece_priority(idx, 7);
    }
    // create a tail of descending priorities
    for(int i = 0; i < num_pieces_in_critical_window; ++i, ++counter) {
        int idx = first_piece_to_prioritize + counter;
        if(idx >= torrent_info.num_pieces()) break;
        torrent_handle_.piece_priority(idx, 6);
    }
    for(int i = 0; i < num_pieces_in_critical_window; ++i, ++counter) {
        int idx = first_piece_to_prioritize + counter;
        if(idx >= torrent_info.num_pieces()) break;
        torrent_handle_.piece_priority(idx, 5);
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
        
        /* for the first buffering_state_length_ pieces, don't delay the
         * random access requests (speeds up pre-buffering) */
        if(buffering_state_counter_ <= buffering_state_length_ || pre_buffer)
        {    
            disp_->post(
                boost::bind(&download_control_worker::fetch_missing_pieces, this, 
                            dev,
                            first_piece_to_prioritize, 
                            first_piece_to_prioritize + num_pieces_in_critical_window - 1,
                            force_request,
                            boost::system::error_code()));
            // explicit pre buffering is not considered part of the (implicit) buffering stage.
            if(!pre_buffer) {
                ++buffering_state_counter_;
#ifdef _DEBUG
                if(buffering_state_counter_ > buffering_state_length_) {
                    BOOST_LOG_TRIVIAL(debug) << "Left buffering state";
                }
#endif
            }
        }
        else
        {
            disp_->post_delayed(
                boost::bind(&download_control_worker::fetch_missing_pieces, this, 
                            dev, 
                            first_piece_to_prioritize, 
                            first_piece_to_prioritize + num_pieces_in_critical_window - 1,
                            force_request,
                            _1));
        }
    }
}

void download_control_worker::fetch_missing_pieces(download_device* dev, 
                                                   int first_piece, 
                                                   int last_piece,
                                                   bool force_request,
                                                   boost::system::error_code& error)
{
    assert(last_piece >= first_piece);
    
    libtorrent::torrent_status status = torrent_handle_.status();
    libtorrent::bitfield pieces = status.pieces;

    assert(last_piece < pieces.size());

    std::vector<libcow::piece_request> reqs;

    const libtorrent::torrent_info& torrent_info = torrent_handle_.get_torrent_info();

    for(int i = first_piece; i <= last_piece; ++i) {
        // don't request non-existing pieces
        if(i >= torrent_info.num_pieces()) {
            break;
        }
        /* Request only pieces that we don't already have or 
         * haven't alread requested. Always request already 
         * requested pieces if force_request is set to true 
         * (but never request pieces that we already have).
         */
        bool shit = !pieces[i];
        if(!pieces[i] && (force_request || !critically_requested_[i])) {
            reqs.push_back(piece_request(torrent_info.piece_length(), i, 1));
            critically_requested_[i] = true;
            BOOST_LOG_TRIVIAL(debug) 
                << "Falling back to random access download devices for piece " << i
                << (force_request ? " (forced request)" : "");
        }
    }

    if(reqs.size() > 0) {
        dev->get_pieces(reqs);
    }
}

void download_control_worker::set_buffering_state()
{
    disp_->post(boost::bind(
        &download_control_worker::handle_set_buffering_state, this));
}

void download_control_worker::handle_set_buffering_state()
{
    BOOST_LOG_TRIVIAL(debug) << "Entered buffering state";
    buffering_state_counter_ = 0;
}
