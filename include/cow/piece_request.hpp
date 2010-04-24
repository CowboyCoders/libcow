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
#include "cow/cow.hpp"
#ifndef ___libcow_piece_request___
#define ___libcow_piece_request___

namespace libcow {

    /**
     * \struct piece_request 
     * This struct contains information about a request for a media piece.
     */
    struct LIBCOW_EXPORT piece_request
    {

       /**
        * Creates a new piece request.
        * @param ps The piece size, i.e size of each torrent piece.
        * @param idx The index of the piece to request.
        * @param c The number of contiguous pieces to download, starting at idx.
        */
        piece_request(const size_t& ps, const size_t& idx, const size_t& c)
            : piece_size(ps)
            , index(idx)
            , count(c)
        {
        }

       /**
        * The size of the piece in bytes.
        */
        size_t piece_size;

       /**
        * The index of the piece.
        */
        size_t index;

       /**
        * Number of pieces in a row to download.
        */
        size_t count;

    };
}
#endif // ___libcow_piece_request___
