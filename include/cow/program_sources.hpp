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

#ifndef __program_sources__
#define __program_sources__

#include "cow/program_info.hpp"

namespace libcow {
    /**
     * Used for keeping track of the ways to download a specific program.
     * This struct contains a list of download_device_descriptions for each
     * program.
     */
    struct program_sources
    {
       /**
        * Creates a new program_sources struct.
        * @param d A vector of download_device_descriptions, describing ways to download this program.
        * @param i The program that can be downloaded using the download_devices described in d.
        */
        program_sources(const std::vector<libcow::download_device_description*>& d, const libcow::program_info& i);
        virtual ~program_sources();

       /**
        * A vector of download_device_description pointer that describes how the
        * program can be downloaded.
        */
        std::vector<libcow::download_device_description*> devices;

       /**
        * A struct containing information about the program.
        */
        libcow::program_info info;
    };
}

#endif
