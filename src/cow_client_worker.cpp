#include "cow/libcow_def.hpp"
#include "cow/cow_client_worker.hpp"

#include "cow/program_info.hpp"

#include <libtorrent/magnet_uri.hpp>

using namespace libcow;

/**
 * This struct is used for comparing a pair on its second value.
 */
template <typename T>
struct match_second 
{
   /**
    * The local value to compare with.
    */
    T val;

   /**
    * Create a new match_second struct to compare values with.
    * @param t The local value to compare with.
    */
    match_second(T const & t) : val (t) {}

   /**
    * Compares a pair.
    * @param p The pair to compare.
    * @return True if the second value compares with the local value.
    */
    template <typename Pair>
    bool operator() (Pair const& p) const
    {
        return (val == p.second);
    }
};

/**
 * A struct that can be used for comparing pointer
 * values.
 */
template <typename T>
struct check_pointer_value
{
   /**
    * The local value to compare to. 
    */
    T* val;

   /**
    * Creates a new check_pointer_value struct for comparing.
    * @param t The value to store locally for comparing.
    */
    check_pointer_value(T* t) : val(t) {}

   /**
    * Compare two pointer values.
    * @param x A pointer to the value to compare with.
    * @return True if the values are equal, otherwise false.
    */
    bool operator() (T* x) const
    {
        return x == val;
    }

};

cow_client_worker::cow_client_worker(libtorrent::session& s)
    : torrent_session_(s),
      download_device_id_(2)
{
    disp_ = new dispatcher(0);
}

cow_client_worker::~cow_client_worker()
{
    delete disp_;
    clear_download_controls();
}

download_control* cow_client_worker::start_download(const program_info& program,
                                       const std::string& download_directory)
{
    boost::function<download_control*()> functor
        = boost::bind<download_control*>(&cow_client_worker::handle_start_download, 
                                         this, 
                                         program, 
                                         download_directory);

    boost::packaged_task<download_control*> task(functor);
    boost::unique_future<download_control*> future = task.get_future();

    disp_->post(boost::bind(&boost::packaged_task<download_control*>::operator(), 
                boost::ref(task)));
    
    future.wait();
    return future.get();
}

download_control* cow_client_worker::handle_start_download(const program_info& program,
                                              const std::string& download_directory)
{
    // begin by checking if this download_control is already active

    download_control_vector::iterator it;
    for(it = download_controls_.begin(); it != download_controls_.end(); ++it) {
        download_control* dc = *it;
        if(dc->id() == program.id) {
            return dc;
        }
    }

    // Look for a torrent device
    device_map::const_iterator device_it = program.download_devices.find("torrent");

    if (device_it == program.download_devices.end()) {
        BOOST_LOG_TRIVIAL(error) << "cow_client: Failed to start download because the program does not have a torrent download device.";
        return 0;
    }

    // Fetch properties for the torrent device 
    properties torrent_props = device_it->second;

    // Create the torrent_handle from the properties
    libtorrent::torrent_handle torrent = create_torrent_handle(torrent_props, download_directory);

    // Make sure the handle is valid
    if (!torrent.is_valid()) {
        BOOST_LOG_TRIVIAL(error) << "cow_client: Failed to create torrent handle.";
        return 0;
    }

    // Create a new download_control
    download_control* download = new libcow::download_control(torrent, 4, 3000, program.id); // FIXME: no magic numbers please :)

    for (device_it = program.download_devices.begin(); 
        device_it != program.download_devices.end(); ++device_it) 
    {
        const std::string& device_type = device_it->first;
		if (device_type != "torrent") {

            // Find the <id, device_type> pair in piece_sources_ with the same type as the device
            std::map<int, std::string>::iterator piece_source_iter;
            piece_source_iter = std::find_if(piece_sources_.begin(), 
                                             piece_sources_.end(),
                                             match_second<std::string>(device_type));

            int device_id = 0;

            if (piece_source_iter != piece_sources_.end()) {
                // Piece source key is the id of the download device
                device_id = piece_source_iter->first; 

                // Get the properties for this downlod device
                const properties& props = device_it->second;

                // Create a new instance of the download device using the factory
                download_device* device = dd_manager_.create_instance(device_id, device_type, props);

                if (!device) {
                    // Write an error but continue since the at least the torrent worked
                    BOOST_LOG_TRIVIAL(error) << "cow_client: Failed to create download device of type: " << device_type;
                } else {
                    libcow::response_handler_function add_pieces_function = 
                        boost::bind(&download_control::add_pieces, download, _1, _2);

                    device->set_add_pieces_function(add_pieces_function);      
                    download->add_download_device(device, device_id, device_type);
                }

            } else {
                BOOST_LOG_TRIVIAL(warning) << "cow_client: Unsupported download device type: " << device_type;
            }

    	}
    }

    download_controls_.push_back(download);
    download_control_for_torrent[torrent] = download;
    return download;
}

void cow_client_worker::remove_download(download_control* download)
{
    disp_->post(boost::bind(
        &cow_client_worker::handle_remove_download, this, download));
}

void cow_client_worker::handle_remove_download(download_control* download)
{
    assert(download != 0);
        
    download_control_vector::iterator iter = 
        std::find_if(download_controls_.begin(), download_controls_.end(), 
                    check_pointer_value<download_control>(download));
    
    if (iter == download_controls_.end()) {
        BOOST_LOG_TRIVIAL(warning) << "cow_client_worker: Can't remove download since it's not started.";
    } else {
        // Remove torrent handle
        torrent_session_.remove_torrent(download->handle_);

        // remove association with torrent handle

        std::map<libtorrent::torrent_handle, download_control*>::iterator mapped_download =
            download_control_for_torrent.find(download->handle_);
        if(mapped_download != download_control_for_torrent.end()) {
            download_control_for_torrent.erase(mapped_download);
        }

        // Remove from the list of controls and free memory
        download_control* dl = *iter;
        download_controls_.erase(iter);

        delete dl;

        BOOST_LOG_TRIVIAL(debug) << "cow_client: Removed program download.";
    }
}

libtorrent::torrent_handle cow_client_worker::create_torrent_handle(const properties& props,
                                                                    const std::string& download_directory)
{
    libtorrent::add_torrent_params params;
    
    if(!download_directory.empty()) {
        params.save_path = download_directory;
    } else {
        BOOST_LOG_TRIVIAL(warning) << "No download directory set, using default path '.'";
        params.save_path = ".";
    }
    
    params.storage_mode = libtorrent::storage_mode_allocate;

    properties::const_iterator torrent_it = props.find("torrent");

    if (torrent_it != props.end()) {

        const std::string& torrent = torrent_it->second;
        size_t timeout = 120; // in seconds, should we be able to configure this?
        std::string torrent_file = http_get_as_string(torrent,timeout); 

    	try {
            // Set the torrent file
    		params.ti = new libtorrent::torrent_info(torrent_file.data(), torrent_file.size());
            // Create and return the torrent_handle
    		return torrent_session_.add_torrent(params);		
    	} 
        catch (libtorrent::libtorrent_exception e) {
    		BOOST_LOG_TRIVIAL(error) << "cow_client: failed to load torrent: " << e.what(); 
    	}
    }

    properties::const_iterator magnet_it = props.find("magnet");

    if (magnet_it != props.end()) {
		try {
            // Make sure there is no torrent file given
			params.ti = 0; 
            // Create a handle from a magnet uri
            return libtorrent::add_magnet_uri(torrent_session_, magnet_it->second, params);
		} 
        catch (libtorrent::libtorrent_exception e) {
            BOOST_LOG_TRIVIAL(error) << "cow_client: Error creating torrent from magnet URI: " << e.what();
		}
    }

    // If we have reached this point we have failed to create the handle
    return libtorrent::torrent_handle();
}

void cow_client_worker::clear_download_controls() 
{
    download_control_vector::iterator iter;
    for(iter = download_controls_.begin(); iter != download_controls_.end(); ++iter) {
        download_control* current = *iter;
        if(current) {
            delete current;
        }
    }
    download_controls_.clear();
}

void cow_client_worker::register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                                         const std::string& identifier)
{
    disp_->post(boost::bind(
        &cow_client_worker::handle_register_download_device_factory, this, factory, identifier));
}

void cow_client_worker::handle_register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                                                const std::string& identifier)
{
    piece_sources_[++download_device_id_] = identifier;
    dd_manager_.register_download_device_factory(factory, identifier);
}

void cow_client_worker::signal_piece_finished(const libtorrent::torrent_handle& torrent, int piece_index)
{
    disp_->post(boost::bind(
        &cow_client_worker::handle_signal_piece_finished, this, torrent, piece_index));
}
void cow_client_worker::handle_signal_piece_finished(const libtorrent::torrent_handle& torrent, int piece_index)
{
    download_control* dev = download_control_for_torrent[torrent];
    if(dev) {
        dev->signal_piece_finished(piece_index);
    }
}

void cow_client_worker::signal_startup_complete(const libtorrent::torrent_handle& torrent)
{
    disp_->post(boost::bind(
        &cow_client_worker::handle_signal_startup_complete, this, torrent));
}
void cow_client_worker::handle_signal_startup_complete(const libtorrent::torrent_handle& torrent)
{
    download_control* dev = download_control_for_torrent[torrent];
    if(dev) {
        dev->signal_startup_complete();
    }
}

std::list<download_control*> cow_client_worker::get_active_downloads()
{
    boost::function<std::list<download_control*>()> functor
    = boost::bind<std::list<download_control*> >(&cow_client_worker::handle_get_active_downloads, this);

    boost::packaged_task<std::list<download_control*> > task(functor);
    boost::unique_future<std::list<download_control*> > future = task.get_future();

    disp_->post(boost::bind(&boost::packaged_task<std::list<download_control*> >::operator(), 
                boost::ref(task)));
    
    future.wait();
    return future.get();
}

std::list<download_control*> cow_client_worker::handle_get_active_downloads()
{
    std::list<download_control*> active_downloads;
    download_control_vector::iterator iter;
    for(iter = download_controls_.begin(); iter != download_controls_.end(); ++iter)
    {
        active_downloads.push_back(*iter);
    }
    return active_downloads;
}
