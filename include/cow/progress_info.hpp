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
#include "libtorrent/session.hpp"
#ifndef ___libcow_progress_info___
#define ___libcow_progress_info___

namespace libcow {
    /**
     * Progress info class. Contains information regarding
     * the progress of some download.
     */
    struct LIBCOW_EXPORT progress_info
    {
       /**
        * An enum describing the state of the current download progress.
        */
        enum torrent_state {
            unknown,
            checking_resume_data,
            queued_for_checking,
            checking_files,
            downloading_metadata,
            downloading,
            finished,
            seeding,
            allocating
        };
       /**
        * Creates a new progress_info and initializes all variables.
        * @param state The current state of the download progress.
        * @param progress The download progress in percent.
        * @param downloaded A bitfield describing whether or not each piece has been downloaded.
        * @param piece_origin A vector describing where each piece originates from.
        */
        progress_info(torrent_state state,
                      float progress,
                      libtorrent::bitfield downloaded,
                      std::vector<int> piece_origin)
            : state_(state),
              progress_(progress),
              downloaded_(downloaded),
              piece_origin_(piece_origin)
        {
        }
       /**
        * Returns the total download progress in percent.
        * @return A value in the range [0, 1] that indicates the progress of the torrent.
        */
        float progress()
        {
            return progress_;
        }

       /**
        * This function returns the string representation of the BitTorrent download process.
        * @return A string with the current state.
        */
        std::string state_str()
        {
            return state2str();
        }

       /**
        * This function returns the current of the BitTorrent download process.
        * @return An enum torrent_state with the current state.
        */
        torrent_state state()
        {
            return state_;
        }

       /**
        * Returns a bitfield with information about which pieces have been downloaded.
        * @return A bitfield where 1 = piece is downloaded, 0 = piece is not downloaded.
        */
        libtorrent::bitfield downloaded()
        {
            return downloaded_;
        }

       /**
        * This function returns information about which source each piece originates from.
        * The number for each source is determined by in which order the download_device
        * factories is registered.
        * @return A vector of piece_origins, where 0 = default(BitTorrent), and subsequent numbers
        * determined by registering download_device factories.
        */
        const std::vector<int>& piece_origin() const
        {
            return piece_origin_;
        }

    private:
        torrent_state state_;
        float progress_;
        libtorrent::bitfield downloaded_;
        std::vector<int> piece_origin_;

        const char* state2str()
        {
            switch(state_) 
            {
            case checking_resume_data: return "Checking resume data";
            case queued_for_checking: return "Queued for checking";
            case checking_files: return "Checking files";
            case downloading_metadata: return "Downloading metadata";
            case downloading: return "Downloading";
            case finished: return "Finished";
            case seeding: return "Seeding";
            case allocating: return "Allocating";
            default: return "Unknown";
            }
        }
    };
}

#endif
