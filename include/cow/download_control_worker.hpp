#ifndef ___libcow_download_control_worker___
#define ___libcow_download_control_worker___

#include <libtorrent/torrent_handle.hpp>
#include <exception>
#include <string>

namespace libcow 
{
    class download_device;
    class dispatcher;
    class chunk;

   /**
    * This class is responsible for carrying out jobs for the
    * libcow::download_control.
    */
    class download_control_worker
    {
    public:
       /**
        * Creates a new download_control_worker.
        * @param h The torrent_handle that this worker belongs to.
        * @param critical_window_length The number of pieces that are critical to download.
        * @param critical_window_timeout The timeout in seconds to use for dispatcher jobs.
        */
        download_control_worker(libtorrent::torrent_handle& h,
                                size_t critical_window_length,
                                int critical_window_timeout);
        ~download_control_worker();

       /**
        * Sets the number of pieces that are critical to download.
        * @param num_pieces The number of critical pieces.
        */
        void set_critical_window(size_t length);
       
        /**
         * This sets the timeout for the pieces in the critical 
         * window. Note that this function is asynchronous, 
         * the timeout will changes as soon as possible
         *
         * @param timeout the new timeout in ms
         */ 
        void set_critical_window_timeout(int timeout);

       /**
        * Adds a new download_device.
        * @param dd A pointer to the download_device.
        */
        void add_download_device(download_device* dd);

       /**
        * Tries to download the specified bytes in a fast manner from random access devices.
        * @param offset The byte offset of the first byte to pre buffer.
        * @param length The number of bytes to pre buffer.
        */
        void pre_buffer(const chunk& c);

        /**
         * This function marks the specified piece as already requested from random access device.
         *
         * @param piece_index the index to marks a not requested
         * @param req The request status
         */
        void set_piece_requested(int piece_index, bool req);

       /**
        * Sets the current byte position for playing back movies. This function is used to
        * indicate wich pieces needs to be downloaded soon.
        * @param offset The current playback position.
        * @param force_request True if we should request pieces that we already have requested before.
        */
        void set_playback_position(size_t offset, bool force_request);
        
        void set_buffering_state();


       /**
        * Returns a map from download_device id to the name of the download_device.
        * @return the map
        */
        std::map<int,std::string> get_device_names();

    private:
        void handle_set_critical_window(size_t length);
        void handle_set_critical_window_timeout(int timeout);
        void handle_add_download_device(download_device* dd);
        void handle_set_playback_position(size_t offset, bool force_request);
        
        void handle_download_strategy(const chunk& c, 
                                      bool force_request, 
                                      bool pre_buffer);
        
        void handle_set_piece_requested(int piece_index, bool req);
        std::map<int,std::string> handle_get_device_names();
        
        void fetch_missing_pieces(download_device* dev,
                                  int first_piece,
                                  int last_piece,
                                  bool force_request,
                                  boost::system::error_code& error);
        void handle_set_buffering_state();

        std::vector<boost::shared_ptr<download_device> > download_devices_;

        std::vector<bool> critically_requested_;
        
        size_t critical_window_;

        libtorrent::torrent_handle torrent_handle_;

        dispatcher* disp_;

        bool is_running_;

        unsigned int buffering_state_counter_;
        static unsigned int buffering_state_length_;
    };
}

#endif // ___libcow_download_control_worker___
