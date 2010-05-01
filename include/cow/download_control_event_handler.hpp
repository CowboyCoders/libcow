#ifndef ___libcow_download_control_event_handler___
#define ___libcow_download_control_event_handler___

#include "cow/dispatcher.hpp"
#include <libtorrent/alert.hpp>
#include <libtorrent/torrent_handle.hpp>

namespace libcow 
{

    class download_control_event_handler
    {
    public:
        download_control_event_handler(libtorrent::torrent_handle& h);
        ~download_control_event_handler();

        void set_piece_src(int source, size_t piece_index);
        void invoke_after_init(boost::function<void(void)> callback);
        void handle_alert(const libtorrent::alert* alert);
        void invoke_when_downloaded(const std::vector<int>& pieces, 
                                    boost::function<void(std::vector<int>)> callback);
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
        void signal_startup_callbacks();
        void set_libtorrent_ready();
        void update_piece_requests(int piece_id);
        std::vector<int> missing_pieces(const std::vector<int>& pieces);

        dispatcher disp_;
        libtorrent::torrent_handle& torrent_handle_;

        std::multimap<int, piece_request> piece_nr_to_request_;

        std::vector<boost::function<void(void)> > startup_complete_callbacks_;
        
        std::vector<int> piece_origin_;

        bool is_libtorrent_ready_;
    };
}

#endif // ___libcow_download_control_event_handler___