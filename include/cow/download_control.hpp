/*
Copyright 2010 CowboyCoders. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COWBOYCODERS ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COWBOYCODERS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of CowboyCoders.
*/

#ifndef ___libcow_download_control___
#define ___libcow_download_control___

#include "cow/utils/buffer.hpp"
#include "dispatcher.hpp"

#include <boost/noncopyable.hpp>
#include <boost/log/trivial.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert.hpp>

namespace libcow {

   /**
    * The purpose of this class is act as the "glue" between different
    * download_devices, libtorrent and the cow_client. This class will
    * implement a callback function that download_devices can use to
    * deliver data to the client. This class is also responsible for
    * determining the priority for each piece, and which download_devices
    * should be used at different scenarios.
    */
    class LIBCOW_EXPORT download_control : public boost::noncopyable
    {
    public:
       /**
        * Creates a new download_control and initializes attributes.
        * @param handle The torrent_handle which controls the BitTorrent
        * part of the download.
        */
        download_control(const libtorrent::torrent_handle& handle, 
                         int critical_window_length,
                         int critical_window_timeout);
        ~download_control();

       /**
        * Creates a new progress_info struct which contains information about
        * the current download progress.
        * @return A progress_info struct with information.
        */
        progress_info get_progress();

        /**
         * @return The number of pieces in the torrent associated with this
         * download_control.
         */
        int num_pieces() 
        {
            return handle_.get_torrent_info().num_pieces();
        }

        void pre_buffer(size_t offset, size_t length);

       /**
        * This function will try to make sure that the requested pieces
        * are downloaded by requesting them from a random access download device.
        * @param requests A vector of requests to download.
        */
        void pre_buffer(const std::vector<libcow::piece_request> requests);

        /**
        * This function returns the total file size of the downloaded data.
        * @return Size of the downloaded data in bytes.
        */
        size_t file_size() const;

        std::string filename() const;

       /**
        * This function reads data from offset into the buffer. The number
        * of bytes to read is determined by the size of the buffer.
        * @param buffer The buffer to read data to.
        * @return The number of bytes actually read.
        */
        size_t read_data(size_t offset, libcow::utils::buffer& buffer);

       /**
        * This function checks whether or not the specified data has been
        * downloaded.
        * @param offset The byte offset of where to start looking for data.
        * @param length How many bytes to check from offset.
        * @return True if the data has been downloaded, otherwise false.
        */
        bool has_data(size_t offset, size_t length);

        /**
         * This function checks wether the supplied pieces are downloaded 
         * and returns a vector of the pieces which aren't.
         *
         * @param pieces The pieces for which the availability shoud be checked for
         * @return a vector that contatins the pieces which aren't downloaded 
         */
        std::vector<int> has_pieces(const std::vector<int>& pieces);

        size_t bytes_available(size_t offset) const;

        /**
         * This function sets the current playback position, i.e the region
         * of the file that libcow should prioritize for download. The piece at
         * 'offset' and a few of the following pieces will be prioritized 
         * for download. The number of pieces to prioritize is set by calling
         * set_critical_window.
         * If the critical window is not filled in time, requests will be
         * made to the random access devices if any such download devices
         * are registered. This timeout can be set when the download_control is
         * created. Set force_request to true to allow pieces that have
         * already been requested to be requested again (useful when recovering
         * from network failures etc.).
         *
         * @param offset The byte offset of the current playback position.
         * @param force_request Allow possibly redundant requests.
         */
        void set_playback_position(size_t offset, bool force_request = false);

       /**
        * Returns the length of a piece.
        * @return The piece length in bytes.
        */
		int piece_length();

       /**
        * This function will be used as a callback for download devices to use
        * when they have finished downloading pieces.
        * @param id The unique id of the download_device.
        * @param pieces A vector of piece_data that has been downloaded.
        */
        void add_pieces(int id, const std::vector<libcow::piece_data>& pieces);

       /**
        * Sets the download device origin for a piece. Since this may happen at any time,
        * this function MUST be called in a threadsafe manner.
        * @param source The unique id of the source. 0 = default(BitTorrent).
        * @param piece_index The index of the piece to set the source for.
        */
        void set_piece_src(int source, size_t piece_index) {
            if(piece_index < piece_origin_.size() &&
                piece_origin_[piece_index] == 0)
            {
                piece_origin_[piece_index] = source;
            }
        }

       /**
        * This function adds a download_device to the vector of download devices
        * that this download_control keeps track of.
        * @param dd The download_device to add.
        */
        void add_download_device(download_device* dd);

        /**
         * The supplied boost::function callback will be called when all the bittorrent pieces in 
         * the supplied std::vector pieces are downloaded. Note that this function is 
         * asynchronous (not blocking)
         *
         * @param callback The function to call when all the pieces are downloaded
         * @param pieces A vector describing which pieces that should be downloaded
         */
        void wait_for_pieces(const std::vector<int>& pieces, boost::function<void(std::vector<int>)> callback);
        
        /**
         * The supplied boost::function callback will be called when the download_control is intitialized and ready.
         * This includes hashing existing files, connecting to a bittorrent tracker etc.
         * Note that this function is asynchronous (not blocking)
         *
         * @param callback The function to call when all the pieces are downloaded
         */
        void wait_for_startup(boost::function<void(void)> callback);

        int critical_window()
        {
            return critical_window_;
        }

        void set_critical_window(int num_pieces)
        {
            critical_window_ = num_pieces;
        }

        void debug_print();

    private:
        void fetch_missing_pieces(download_device* dev, 
                                  int first_piece, 
                                  int last_piece,
                                  bool force_request,
                                  boost::system::error_code& error);
        
        // used by cow_client when a new libtorrent::alert arrives.
        // this is possible since cow_client is friend
        // with download_control. we don't want to make this public.
        void handle_alert(const libtorrent::alert* event);

        void update_piece_requests(int piece_id);
        void signal_startup_callbacks();
        
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
        
        std::multimap<int, piece_request> piece_nr_to_request_;
        
        std::vector<boost::function<void(void)> > startup_complete_callbacks_;

        std::map<int,std::string> * piece_sources_;
        std::vector<int> piece_origin_;
        libtorrent::torrent_handle handle_;

        std::vector<boost::shared_ptr<download_device> > download_devices_;

        std::ifstream file_handle_;

        dispatcher disp_;


        int critical_window_;
        std::vector<bool> critically_requested_;
        
        bool is_libtorrent_ready_;

        // cow_client should have access to the torrent_handle
        friend class libcow::cow_client;

    };
}

#endif // ___libcow_download_control___
