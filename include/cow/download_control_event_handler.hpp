#ifndef ___libcow_download_control_event_handler___
#define ___libcow_download_control_event_handler___

#include "cow/dispatcher.hpp"
#include <libtorrent/alert.hpp>
#include <libtorrent/torrent_handle.hpp>

namespace libcow 
{

   /**
    * This class handles events from libcow::download_control:s.
    */
    class download_control_event_handler
    {
    public:
       /**
        * Creates a new download_control_event_handler and initializes some variables.
        * @param h The torrent_handle that this download_control_event_handler belongs to.
        */
        download_control_event_handler(libtorrent::torrent_handle& h);
        ~download_control_event_handler();
    
       /**
        * Sets the piece_origin for a piece. This function is asynchronous.
        * @param source The source of the piece.
        * @param piece_index The index of the piece to set the source for.
        */
        void set_piece_src(int source, size_t piece_index);

       /**
        * This function calls the callback if libtorrent is ready. Otherwise,
        * it will be added to a queue and called later when libtorrent is ready.
        * @param callback The function to call.
        */
        void invoke_after_init(boost::function<void(void)> callback);

       /**
        * This function calls the callback function if libtorrent has completely finished
        * downloading the specified pieces.
        * @param piece The pieces that needs to be downloaded.
        * @param callback The callback function to call.
        */
        void invoke_when_downloaded(const std::vector<int>& pieces, 
                                    boost::function<void(std::vector<int>)> callback);

       /** 
        * This function signals the we are ready to start downloading.
        */
        void signal_startup_complete();

       /**
        * This function signals that we have finished downloading the specified piece.
        * @param piece_index The index of the piece that we have finished downloading.
        */
        void signal_piece_finished(int piece_index);

       /**
        * This function sets the callback to call when we have finished downloading pieces.
        * @param func The function to call.
        */
        void set_piece_finished_callback(const boost::function<void(int,int)>& func);

       /**
        * Removes the piece_finished_callback.
        */
        void unset_piece_finished_callback();

       /**
        * Fills the specified vector with piece_origin data. This function
        * is blocking.
        * @param state The vector to fill with data.
        * @return True if it was possible to retrieve the piece_origins,
        * otherwise false.
        */
        bool get_current_state(std::vector<int>& state);
    private:
        class piece_request
        {
        public:
            typedef boost::function<void(std::vector<int>)> callback;
            
            piece_request(std::list<int>* pieces, callback func, std::vector<int>* org_pieces)
                : pieces_(pieces),
                  callback_(func),
                  org_pieces_(org_pieces) {} // empty
            
            std::list<int>* pieces_;
            callback callback_;
            std::vector<int>* org_pieces_;
        };

        void handle_set_piece_src(int source, size_t piece_index);
        void handle_invoke_after_init(boost::function<void(void)> callback);
        void handle_invoke_when_downloaded(const std::vector<int>& pieces, 
                                           boost::function<void(std::vector<int>)> callback);
        void handle_set_piece_finished_callback(const boost::function<void(int,int)>& func);
        void handle_unset_piece_finished_callback();
        bool handle_get_current_state(std::vector<int>& state);
        void signal_startup_callbacks();
        void handle_signal_startup_complete();
        void update_piece_requests(int piece_id);
        void invoke_piece_finished_callback(int piece_index, int device);
        std::vector<int> missing_pieces(const std::vector<int>& pieces);
        void set_disk_source();

        dispatcher* disp_;
        dispatcher* callback_worker_;
        libtorrent::torrent_handle& torrent_handle_;

        std::multimap<int, piece_request> piece_nr_to_request_;

        std::vector<boost::function<void(void)> > startup_complete_callbacks_;
        boost::function<void(int,int)> piece_finished_callback_;

        std::vector<int> piece_origin_;

        bool is_libtorrent_ready_;
    };
}

#endif // ___libcow_download_control_event_handler___
