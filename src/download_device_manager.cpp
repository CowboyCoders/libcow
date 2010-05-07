/*
Copyright 2010 CowboyCoders. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COWBOYCODERS ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL COWBOYCODERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of CowboyCoders.
*/

#include "cow/libcow_def.hpp"
#include "cow/download_device_manager.hpp"
#include "cow/download_device_factory.hpp"

using namespace libcow;

void download_device_manager::register_download_device_factory(
    boost::shared_ptr<download_device_factory> factory, 
    const std::string& identifier) 
{   
    factories[identifier] = factory;
}   

download_device* download_device_manager::create_instance(int id, std::string ident, const properties& pmap)
{   
   std::map<std::string, boost::shared_ptr<download_device_factory> >::iterator it; 
   it = factories.find(ident);
   if(it == factories.end()) {
       return 0;
   } else {
       boost::shared_ptr<download_device_factory> factory_ptr = it->second;
       download_device_factory* factory = factory_ptr.get();
       if(!factory) {
           std::stringstream ss;
           ss << "Could not get download device factory with id: " << id << " and ident: " << ident;
           throw libcow::exception(ss.str());
       }
       return factory->create(id, ident, pmap);
   }
}

