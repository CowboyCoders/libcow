#ifndef ___libcow_cow_client_worker___
#define ___libcow_cow_client_worker___

#include "cow/dispatcher.hpp"
#include "cow/download_device_manager.hpp"

#include <libtorrent/alert.hpp>
#include <libtorrent/session.hpp>

#include <boost/thread/future.hpp>

#include <string>

namespace libcow 
{
    struct program_info;
    class download_control;
   /**
    * This class is responsible for carrying out jobs for the
    * libcow::cow_client.
    */
    class LIBCOW_EXPORT cow_client_worker
    {
    public:
       /**
        * Creates a new worker for the specified session.
        * @param s The libtorrent::session that this worker belongs to.
        */
        cow_client_worker(libtorrent::session& s);
        ~cow_client_worker();

       /**
        * Starts a new download of the specified program to the specified directory.
        * This function is blocking.
        * @param program The program to start downloading.
        * @param download_directory The path to download files to.
        * @return A pointer to the libcow::download_control for this program.
        */
        download_control* start_download(const program_info& program,
                                         const std::string& download_directory);
        
       /**
        * Stops and removes the download. This function is asynchronous.
        * @param download The libcow::download_control pointer to the download. 
        */
        void remove_download(download_control* download);
       
       /**
        * This function registers a new download_device_factory which can be used
        * for creating new download_devices.
        * @param factory A boost::shared_ptr to the factory to register.
        * @param identifier A string describing the download_device, e.g. "http", "multicast" etc.
        */
        void register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                              const std::string& identifier);

       /**
        * This function signals to the specified torrent_handle that we have finished
        * downloading a piece. This functions is asynchronous.
        * @param torrent The torrent_handle to send the signal to,
        * @param piece_index The index of the piece that we have finished downloading.
        */
        void signal_piece_finished(const libtorrent::torrent_handle& torrent, int piece_index);

       /**
        * This function signals that we are ready to start downloading data.
        * @param torrent The torrent_handle to send the signal to.
        */
        void signal_startup_complete(const libtorrent::torrent_handle& torrent);

       /**
        * Returns a list of the current active downloads. This function is blocking.
        * @return A list of libcow::download_control pointers to the current active downloads.
        */
        std::list<download_control*> get_active_downloads();

        /**
         * Calls the correct download_control that the hash of the piece has failed
         *
         * @param torrent_handle the handle for which the piece hash failed
         * @param piece_index the index of piece
         */
        void signal_hash_failed(const libtorrent::torrent_handle& handle, int piece_index);

    private:
        download_control* handle_start_download(const program_info& program,
                                                const std::string& download_directory);
        
        void handle_remove_download(download_control* download);
        
        void handle_register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                                     const std::string& identifier);
        
        void handle_signal_piece_finished(const libtorrent::torrent_handle& torrent, int piece_index);
        void handle_signal_startup_complete(const libtorrent::torrent_handle& torrent);
        void handle_signal_hash_failed(const libtorrent::torrent_handle& handle, int piece_index);
        std::list<download_control*> handle_get_active_downloads();

        void clear_download_controls();
        libtorrent::torrent_handle create_torrent_handle(const properties& props,
                                                         const std::string& download_directory);

        dispatcher* disp_;

        libtorrent::session& torrent_session_;

        typedef std::vector<download_control*> download_control_vector;
        download_control_vector download_controls_;

        download_device_manager dd_manager_;
        
        // should only be accessed via alert_disp_
        std::map<libtorrent::torrent_handle, download_control*>
            download_control_for_torrent;

        std::map<int,std::string> piece_sources_;

        // autoincremented id for new download devices
        int download_device_id_;

    };
}

#endif // ___libcow_cow_client_worker___
