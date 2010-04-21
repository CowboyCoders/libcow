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
    struct progress_info
    {
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

        float progress()
        {
            return progress_;
        }

        std::string state_str()
        {
            return state2str();
        }

        torrent_state state()
        {
            return state_;
        }

        libtorrent::bitfield downloaded()
        {
            return downloaded_;
        }

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
