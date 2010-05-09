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

#include <cow/utils/buffer.hpp>
#include <cow/utils/chunk.hpp>
#include <cow/dispatcher.hpp>
#include <cow/download_control_event_handler.hpp>
#include <cow/download_control_worker.hpp>
#include <cow/exceptions.hpp>

#include <boost/noncopyable.hpp>
#include <boost/log/trivial.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert.hpp>
#include <boost/thread.hpp>

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
        * @param critical_window_length HENRY, document this
        * @param critical_window_timeout HENRY, document this
        * @param id a unique id for this download device
        * @param download_directory the directory for the file
        */
        download_control(const libtorrent::torrent_handle& handle, 
                         int critical_window_length,
                         int critical_window_timeout,
                         int id,
                         std::string download_directory);
        ~download_control();

        /**
         * @return The number of pieces in the torrent associated with this
         * download_control.
         */
        int num_pieces() 
        {
            return handle_.get_torrent_info().num_pieces();
        }

        /**
         * This sets the timeout for the pieces in the critical 
         * window. Note that this function is asynchronous, 
         * the timeout will changes as soon as possible
         *
         * @param timeout the new timeout in ms
         */ 
        void set_critical_window_timeout(int timeout)
        {
            worker_->set_critical_window_timeout(timeout);
        }

       /**
        * This function will try to make sure that the requested pieces
        * are downloaded by requesting them from a random access download device.
        * @param requests A vector of requests to download.
        */
        void pre_buffer(const chunk& c)
        {
            worker_->pre_buffer(c);
        }

        void pre_buffer(const std::vector<chunk>& chunks, boost::function<void(std::vector<int>)> callback)
        {
            std::vector<chunk>::const_iterator it;
            for(it = chunks.begin(); it != chunks.end(); ++it) {
                pre_buffer(*it);
            }
            invoke_when_downloaded(chunks, callback);
        }


        /**
        * This function returns the total file size of the downloaded data.
        * @return Size of the downloaded data in bytes.
        */
        size_t file_size() const;

       /**
        * Returns the filename for this download.
        * @return The filename.
        */
        std::string filename() const;

       /**
        * This function reads data from offset into the buffer. The number
        * of bytes to read is determined by the size of the buffer.
        * @param offset The byte offset to start reading from.
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
         * The number of sequential bytes available from 
         * the position offset.
         *
         * @param offset the position in the file (in bytes) from
         *               where the count should start.
         * @return the number of bytes
         */
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
        void set_playback_position(size_t offset, bool force_request = false) {
            worker_->set_playback_position(offset, force_request);
        }

       /**
        * Returns the length of a piece.
        * @return The piece length in bytes.
        */
		int piece_length();

       /**
        * Returns the size of the file managed by this download_control.
        * @returns The file size in bytes.
        */
        size_t file_size() 
        {
            const libtorrent::torrent_info& torrent_info = handle_.get_torrent_info();
            if(torrent_info.num_files() != 1) {
                std::stringstream ss;
                ss << "Wrong number of files in this torrent. Expected 1 but was " << torrent_info.num_files();
                std::string msg = ss.str();
                throw libcow::exception(msg);
            }

            return static_cast<size_t>(torrent_info.file_at(0).size);
        }

       /**
        * This function will be used as a callback for download devices to use
        * when they have finished downloading pieces.
        * @param id The unique id of the download_device.
        * @param pieces A vector of piece_data that has been downloaded.
        */
        void add_pieces(int id, const std::vector<libcow::piece_data>& pieces);

       /**
        * This function adds a download_device to the vector of download devices
        * that this download_control keeps track of.
        * @param dd The download_device to add.
        * @param id The unique id of the device.
        * @param name The name of the device.
        */
        void add_download_device(download_device* dd);

        /**
         * The supplied boost::function callback will be called when all the bittorrent pieces in 
         * the supplied std::vector 'pieces' are downloaded. Note that this function is 
         * asynchronous (not blocking)
         *
         * @param callback The function to call when all the pieces are downloaded
         * @param pieces A vector describing which pieces should be downloaded
         */
        void invoke_when_downloaded(const std::vector<chunk>& chunks, 
                                    boost::function<void(std::vector<int>)> callback)
        {
            event_handler_->invoke_when_downloaded(chunks, callback);
        }
        
        /**
         * The supplied boost::function callback will be called when the download_control is intitialized and ready.
         * This includes hashing existing files, connecting to a bittorrent tracker etc.
         * Note that this function is asynchronous (not blocking)
         *
         * @param callback The function to call when all the pieces are downloaded
         */
        void invoke_after_init(boost::function<void(void)> callback) 
        {
            event_handler_->invoke_after_init(callback);
        }

        /**
         * Sets the number of consecutive bytes, starting with the byte at the
         * current playback position, that should be prioritized for downloading.
         *
         * @param length The length, in bytes, of the critical window
         */
        void set_critical_window(size_t length)
        {
            worker_->set_critical_window(length);
        }

        /**
        * Adds a function to be called when new pieces are added.
        * @param func A callback function of type void(int).
        */
        void set_piece_finished_callback(const boost::function<void(int,int)>& func) {
            event_handler_->set_piece_finished_callback(func);
        }

        /**
        * Removes a 'piece finished' callback.
        * @param func A callback function of type void(int).
        */
        void unset_piece_finished_callback() {
            event_handler_->unset_piece_finished_callback();
        }

        /**
         * Calling this functions is a hint to the download control that
         * we want pieces fast and don't want to time out before hitting
         * the random access download devices.
         */
        void set_buffering_state() {
            worker_->set_buffering_state();
        }
        /**
         * Prints useful debug information
         */
        void debug_print();

        /**
         * Returns the id of this download_control.
         * The id is unique for a download_control.
         *
         * @return the id
         */
        int id()
        {
            return id_;
        }

       /**
        * Fills the specified vector with piece_origin data. This function
        * is blocking.
        * @param state The vector to fill with data.
        * @return True if it was possible to retrieve the piece_origins,
        * otherwise false.
        */
        bool get_current_state(std::vector<int>& state);

       /**
        * Returns a map from download_device id to the name of the download_device.
        * @return the map
        */
        std::map<int,std::string> get_device_names();

    private:
        void signal_startup_complete() {
            event_handler_->signal_startup_complete();
        }
        
        void signal_piece_finished(int piece_index) {
            event_handler_->signal_piece_finished(piece_index);
        }

        void handle_hash_failed(int piece_index) {
            event_handler_->handle_hash_failed(piece_index);
            worker_->set_piece_requested(piece_index, false);
        }

        void set_piece_src(int source, size_t piece_index) {
            event_handler_->set_piece_src(source, piece_index);
        }
        
        libtorrent::torrent_handle handle_;
        download_control_event_handler* event_handler_;
        download_control_worker* worker_;
        
        std::ifstream file_handle_;

        int id_;
        std::string download_dir_;

        friend class cow_client_worker;
    };
}

#endif // ___libcow_download_control___
