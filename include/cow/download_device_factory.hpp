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
#ifndef ___libcow_download_device_factory___
#define ___libcow_download_device_factory___

namespace libcow {

   /**
    * This interface must be implemented for each download_device in order
    * to be able to create new download_devices dynamically.
    */
	struct LIBCOW_EXPORT download_device_factory {
        virtual ~download_device_factory() {}
       /**
        * This function creates a new download device of the type that implements
        * the interface. A pointer to the new object is returned. The
        * properties for the specific download device is sent using a properties
        * map. Ownership of the returned object is passed to the caller, that is, whoever
        * called this function is responsible for deleting the download_device.
        * @param id The unique id for this device.
        * @param pmap A properties map with properties for the download_device.
        * @return A pointer to the newly created download_device.
		* @author crimzor
		*/
        virtual download_device * create(int id, const properties& pmap) = 0;
	};
}

#endif // ___libcow_download_device_factory___
