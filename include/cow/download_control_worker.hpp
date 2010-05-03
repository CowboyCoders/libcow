#ifndef ___libcow_download_control_worker___
#define ___libcow_download_control_worker___

#include <libtorrent/torrent_handle.hpp>

namespace libcow 
{
    class download_device;
    class dispatcher;

    class download_control_worker
    {
    public:
        download_control_worker(libtorrent::torrent_handle& h,
                                int critical_window_length,
                                int critical_window_timeout);
        ~download_control_worker();

        void set_critical_window(int num_pieces);
        void add_download_device(download_device* dd);
        void pre_buffer(const std::vector<libcow::piece_request>& requests);
        void set_playback_position(size_t offset, bool force_request);

    private:
        void handle_set_critical_window(int num_pieces);
        void handle_add_download_device(download_device* dd);
        void handle_pre_buffer(const std::vector<libcow::piece_request>& requests);
        void handle_set_playback_position(size_t offset, bool force_request);
        
        void fetch_missing_pieces(download_device* dev,
                                  int first_piece,
                                  int last_piece,
                                  bool force_request,
                                  boost::system::error_code& error);

        std::vector<boost::shared_ptr<download_device> > download_devices_;

        std::vector<bool> critically_requested_;
        
        int critical_window_;

        libtorrent::torrent_handle torrent_handle_;

        dispatcher* disp_;
    };
}

#endif // ___libcow_download_control_worker___