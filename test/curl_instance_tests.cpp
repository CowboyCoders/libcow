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

#include <iostream>
#include <string>
#include <tinyxml.h>

using libcow::http_get_as_string;
using libcow::program_info;
using libcow::properties;

program_info* parse_program_info(TiXmlElement* p) {
    program_info* pi = new program_info();
    if(p->QueryIntAttribute("id", &pi->id) != TIXML_SUCCESS)
        std::cerr << "Invalid program: attribute missing\n";

    return pi;
}

int main(int argc, char* argv[]) {
    std::string program_table = http_get_as_string("cowboycoders.se/program_table.xml");
    std::cout << "download size:" << program_table.length() << "\n";

    std::cout << program_table << "\n";

    std::cout << "-\n";
    TiXmlDocument doc;
    doc.Parse(program_table.c_str());
    if(doc.Error())
        std::cout << "Parse error:" << doc.ErrorDesc() << "\n";

    std::cout << "--\n";

    TiXmlHandle docHandle( &doc );
    // <program_id, <download_device_type, properties> >
    std::map<int, std::map<std::string, properties> > download_device_information_map;
    TiXmlElement* child = docHandle.FirstChild( "program_table" ).FirstChild( "program" ).ToElement();
    for( ; child; child=child->NextSiblingElement() )
	{
        program_info* pi(parse_program_info(child));
        if(!(pi->id)){
            continue;
        }
        std::cout << "program_id = " << pi->id << std::endl;
        TiXmlHandle tmp_handle(child);
        TiXmlElement* child2 = tmp_handle.FirstChild("download_devices").FirstChild("device").ToElement();
        std::map<std::string, properties> device_prop_map;
        for( ; child2; child2=child2->NextSiblingElement() )
        {
            std::string type;
            if(child2->QueryStringAttribute("type", &type) != TIXML_SUCCESS){
                std::cerr << "Invalid device: attribute missing\n";
                continue;
            }

            std::cout << "type: " << type << "\n";
            TiXmlHandle tmp_handle2(child2);
            TiXmlElement* child3 = tmp_handle2.FirstChild("property").ToElement();
            properties props;
            for( ; child3; child3=child3->NextSiblingElement() )
            {
                 std::string prop_name;
                 if(child3->QueryStringAttribute("name", &prop_name) != TIXML_SUCCESS){
                    std::cerr << "Invalid property: attribute missing\n";
                    continue;
                 }
                 props[prop_name] = child3->GetText();
            }
            device_prop_map[type] = props;
        }
        download_device_information_map[pi->id] = device_prop_map;
	}
	
    std::cout << ((download_device_information_map[1])["http"])["port"];
    /*
    while(c = root->IterateChildren(c)) {
        if(c->Type() == c->TINYXML_ELEMENT) {
            //program_info* pi = parse_program_info(c->ToElement());
            int program_id = -1;
            if(p->QueryIntAttribute("id", &program_id) != TIXML_SUCCESS)
                std::cerr << "Invalid program: attribute missing\n";
            while (d = c->IterateChildren(d)){
                if(d->Type() == d->TINYXML_ELEMENT) {
                    
                }
            }
            std::cout << "Program id: " << program_id << "\n";
        }

        std::cout << c->Value() << "\n";
    }
    */
	system("PAUSE");
}
