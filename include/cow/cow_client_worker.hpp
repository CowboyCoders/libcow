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

    class LIBCOW_EXPORT cow_client_worker
    {
    public:
        cow_client_worker(libtorrent::session& s);
        ~cow_client_worker();

        download_control* start_download(const program_info& program,
                                         const std::string& download_directory);
        
        void remove_download(download_control* download);
        
        void register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                              const std::string& identifier);

        void signal_piece_finished(const libtorrent::torrent_handle& torrent, int piece_index);
        void signal_startup_complete(const libtorrent::torrent_handle& torrent);
        std::list<download_control*> get_active_downloads();

    private:
        download_control* handle_start_download(const program_info& program,
                                                const std::string& download_directory);
        
        void handle_remove_download(download_control* download);
        
        void handle_register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
                                                     const std::string& identifier);
        
        void handle_signal_piece_finished(const libtorrent::torrent_handle& torrent, int piece_index);
        void handle_signal_startup_complete(const libtorrent::torrent_handle& torrent);
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