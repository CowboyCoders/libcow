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

#ifndef ___on_demand_server_connection_factory___
#define ___on_demand_server_connection_factory___

#include "cow/download_device_factory.hpp"

namespace libcow
{
   /**
    * This factory is used to dynamically create on_demand_server_connections.
    */
	struct LIBCOW_EXPORT on_demand_server_connection_factory 
        : public download_device_factory 
    {
        virtual ~on_demand_server_connection_factory() {}
       /**
        * This function creates a new on_demand_server_connection.
        * A pointer to the new object is returned. The
        * properties for the specific download device is sent using a properties
        * map. Ownership of the returned object is passed to the caller, that is, whoever
        * called this function is responsible for deleting the download_device.
        * @param id The unique id for this device.
        * @param type The type for this download_device
        * @param pmap A properties map with properties for the download_device.
        * @return A pointer to the newly created download_device.
        * @author crimzor
		*/
        virtual download_device * create(int id, std::string type, const properties& pmap);
	};
}

#endif // ___on_demand_server_connection_factory___
