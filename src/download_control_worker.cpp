#include "cow/libcow_def.hpp"
#include "cow/download_control_worker.hpp"
#include "cow/download_device.hpp"
#include "cow/piece_request.hpp"
#include "cow/dispatcher.hpp"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using namespace libcow;

download_control_worker::download_control_worker(libtorrent::torrent_handle& h,
                                                 int critical_window_length,
                                                 int critical_window_timeout)
    : torrent_handle_(h),
      critical_window_(critical_window_length)
{
    critically_requested_ = 
        std::vector<bool>(torrent_handle_.get_torrent_info().num_pieces(), false);
    disp_ = new dispatcher(critical_window_timeout);
}

download_control_worker::~download_control_worker()
{
    delete disp_;
    download_devices_.clear();
}

void download_control_worker::set_critical_window(int num_pieces)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_set_critical_window, this, num_pieces));
}

void download_control_worker::handle_set_critical_window(int num_pieces)
{
    critical_window_ = num_pieces;
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

void download_control_worker::pre_buffer(const std::vector<int>& requests)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_pre_buffer, this, requests));

}

void download_control_worker::handle_pre_buffer(const std::vector<int>& requests)
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
        int piece_length = torrent_handle_.get_torrent_info().piece_length();
        std::vector<piece_request> wrapped_request;
        std::vector<int>::const_iterator requests_iter;
        for(requests_iter = requests.begin(); requests_iter != requests.end(); ++requests_iter) {
            wrapped_request.push_back(piece_request(piece_length, *requests_iter, 1));
        }
        random_access_device->get_pieces(wrapped_request);
    } else {
        BOOST_LOG_TRIVIAL(info) << "Can't pre_buffer. No random access device available.";
    }
}

void download_control_worker::set_playback_position(size_t offset, bool force_request)
{
    disp_->post(boost::bind(
        &download_control_worker::handle_set_playback_position, this, offset, force_request));
}

void download_control_worker::handle_set_playback_position(size_t offset, bool force_request)
{
    // don't fiddle with download strategies if seeding
    if(torrent_handle_.status().state == libtorrent::torrent_status::seeding) {
        return;
    }

    assert(offset >= 0);

    // create a vector of pieces to prioritize
    int first_piece_to_prioritize = offset / torrent_handle_.get_torrent_info().piece_length();

    //std::cout << "first_piece_to_prioritize: " << first_piece_to_prioritize << std::endl;

    // set low priority for pieces before playback position
    for(int i = 0; i < first_piece_to_prioritize; ++i) {
        torrent_handle_.piece_priority(i, 1);
    }

    // set high priority for pieces in critical window
    for(int i = 0; i < critical_window_; ++i) {
        int idx = first_piece_to_prioritize + i;
        if(idx >= torrent_handle_.get_torrent_info().num_pieces()) break;
        torrent_handle_.piece_priority(idx, 7);
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
        disp_->post_delayed(
            boost::bind(&download_control_worker::fetch_missing_pieces, this, 
                        dev, 
                        first_piece_to_prioritize, 
                        first_piece_to_prioritize + critical_window_ - 1,
                        force_request,
                        _1));
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

    libtorrent::torrent_info torrent_info = torrent_handle_.get_torrent_info();

    for(int i = first_piece; i <= last_piece; ++i) {
        // don't request non-existing pieces
        if(i >= torrent_handle_.get_torrent_info().num_pieces()) {
            break;
        }
        /* Request only pieces that we don't already have or 
         * haven't alread requested. Always request already 
         * requested pieces if force_request is set to true 
         * (but never request pieces that we already have).
         */
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
