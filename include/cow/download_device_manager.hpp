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

#ifndef ___libcow_download_device_manager___
#define ___libcow_download_device_manager___

#include <boost/shared_ptr.hpp>

#include "libcow_def.hpp"

namespace libcow {

    class LIBCOW_EXPORT download_device_manager 
    {
    public:
        /** \fn Register a download device factory.
        * Any previously registered factory with the same unique identifier
        * will be removed!
        */
        void register_download_device_factory(boost::shared_ptr<download_device_factory> factory, 
            const std::string& identifier); 

        /** @return A new download_device of some type specified by the unique identifier.
        * Returns 0 if no such download_device type is registered.
        */
        download_device* create_instance(int id, std::string ident, const properties& pmap);

        //TODO: delete_instance ?

    private:
        std::map<std::string, boost::shared_ptr<download_device_factory> > factories;
    };
}

#endif // ___libcow_download_device_manager___
