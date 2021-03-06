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
#include "cow/program_table.hpp"
#include "cow/curl_instance.hpp"
#include "cow/program_info.hpp"

#include "tinyxml.h"

#include <boost/log/trivial.hpp>

using namespace libcow;

const char* xml_programtable_root = "program_table";
const char* xml_programtable_program_tag = "program";

const char* xml_program_name_tag = "name";
const char* xml_program_description_tag = "description";
const char* xml_program_downloaddevices_tag = "download_devices";
const char* xml_program_id_attribute = "id";

const char* xml_downloaddevices_device_tag = "device";
const char* xml_device_property_tag = "property";
const char* xml_device_type_attribute = "type";

const char* xml_property_name_attribute = "name";

void log_program_table_attribute_missing(const std::string& attribute)
{
    BOOST_LOG_TRIVIAL(warning) << "program_table: Attribute \"" << attribute << "\" missing in program table.\n";
}

bool parse_device(TiXmlElement* device_node, std::string& type, libcow::properties& props)
{
    if (device_node->QueryStringAttribute(xml_device_type_attribute, &type) != TIXML_SUCCESS) {
        BOOST_LOG_TRIVIAL(warning) << "program_table: Attribute \"" << xml_device_type_attribute << "\" missing on device in program table.\n";
        return false;
    }

    TiXmlElement* device_prop = device_node->FirstChildElement(xml_device_property_tag);
    for (; device_prop; device_prop = device_prop->NextSiblingElement()) {
        std::string prop_name;
        if (device_prop->QueryStringAttribute(xml_property_name_attribute, &prop_name) != TIXML_SUCCESS) {
            BOOST_LOG_TRIVIAL(warning) << "program_table: Attribute \"" << xml_property_name_attribute << "\" missing on device property in program table.\n";
            return false;
        }

        props[prop_name] = device_prop->GetText();
    }

    return true;
}

bool parse_program_info(TiXmlElement* program_node, program_info& dest)
{
    assert(program_node != 0);

    //
    // Parse general program info
    //

    if (program_node->QueryIntAttribute(xml_program_id_attribute, &dest.id) != TIXML_SUCCESS) {
        BOOST_LOG_TRIVIAL(warning) << "program_table: Attribute \"" << xml_program_id_attribute << "\" missing in program table.\n";
        return false;
    }

    TiXmlElement* name_elem = program_node->FirstChildElement(xml_program_name_tag);
    if (!name_elem) {
        BOOST_LOG_TRIVIAL(warning) << "program_table: Element \"" << xml_program_name_tag << "\" missing in program table.\n";
        return false;
    }

    TiXmlElement* desc_elem = program_node->FirstChildElement(xml_program_description_tag);
    if (!desc_elem) {
        BOOST_LOG_TRIVIAL(warning) << "program_table: Element \"" << xml_program_description_tag << "\" missing in program table.\n";
        return false;
    }

    dest.name = name_elem->GetText();
    dest.description = desc_elem->GetText();

    //
    // Parse device list
    //

    dest.download_devices.clear();

    TiXmlElement* device_node = TiXmlHandle(program_node).
        FirstChild(xml_program_downloaddevices_tag).FirstChild(xml_downloaddevices_device_tag).ToElement();

    for(; device_node; device_node = device_node->NextSiblingElement())
    {
        std::string type;
        properties props;
        if (parse_device(device_node, type, props)) {
            dest.download_devices[type] = props;
        } else {
            BOOST_LOG_TRIVIAL(warning) << "program_table: Skipping invalid device.";
        }
    }

    if (dest.download_devices.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "program_table: Program didn't contain any valid download devices.\n";
        return false;
    }

    return true;
}

void program_table::load_from_file(const std::string& file_name)
{
    std::ifstream fs(file_name.c_str(), std::ios_base::in);
    if (!fs) {
        std::stringstream ss;
        ss << "Error opening file \"" << file_name << "\".";
        BOOST_LOG_TRIVIAL(error) << "program_table: " << ss;
        throw libcow::exception(ss.str());
    }

    fs.seekg(0, std::ios_base::end);
    std::streampos length = fs.tellg();
    fs.seekg(0);

    if (length <= 0) {
        std::stringstream ss;
        ss << "Failed to seek in file: " << file_name;
        throw libcow::exception(ss.str());
    }

    // Allocate and read the whole file in one read
    char* buffer = new char[length];
    fs.read(buffer, length);

    // Copy buffer to a string
    std::string s(buffer);
    delete [] buffer;

    load_from_string(s);    
}

void program_table::load_from_http(const std::string& url, size_t timeout)
{
    curl_instance curl(url);
    std::stringstream& conf_stream = curl.perform_unbounded_request(timeout,std::vector<std::string>());
    BOOST_LOG_TRIVIAL(debug) << "program_table: downloaded program table with size:" << conf_stream.str().length();
    load_from_string(conf_stream.str());
}

void program_table::load_from_string(const std::string& s)
{
    // Clear old program table entries if any
    clear();

    TiXmlDocument doc;
    doc.Parse(s.c_str());

    if (doc.Error()) {
        std::stringstream ss;
        ss << "Parse error: " << doc.ErrorDesc();
        BOOST_LOG_TRIVIAL(error) << "program_table: " << ss;
        throw libcow::exception(ss.str());        
    }

    TiXmlHandle docHandle(&doc);
    TiXmlElement* program_node = docHandle.FirstChild(xml_programtable_root).FirstChild(xml_programtable_program_tag).ToElement();
    for(; program_node; program_node = program_node->NextSiblingElement())
	{
        program_info prog_info;
        if (parse_program_info(program_node, prog_info)) {
            add(prog_info);
        } else {
            BOOST_LOG_TRIVIAL(warning) << "program_table: Skipping invalid program.";
        }
	}
}

void program_table::clear()
{
    entries_.clear();
}

size_t program_table::size() const
{
    return entries_.size();
}

void program_table::add(const libcow::program_info& entry)
{
    entries_.push_back(entry);
}

libcow::program_info_vector::const_iterator program_table::find(int program_id) const {
	libcow::program_info_vector::const_iterator it;
	for (it = entries_.begin(); it != entries_.end(); ++it){
		if (it->id == program_id){
			return it;
		}
	}
	return it;
}

const libcow::program_info& program_table::at(size_t index) const
{
    assert(index >= 0 && index < size());
    return entries_[index];
}

libcow::program_info& program_table::at(size_t index)
{
    assert(index >= 0 && index < size());
    return entries_[index];
}

const libcow::program_info& program_table::operator[](size_t index) const
{
    assert(index >= 0 && index < size());
    return entries_[index];
}

libcow::program_info& program_table::operator[](size_t index)
{
    assert(index >= 0 && index < size());
    return entries_[index];
}

program_table::iterator program_table::begin() 
{
    return entries_.begin();
}

program_table::const_iterator program_table::begin() const
{
    return entries_.begin();
}

program_table::iterator program_table::end()
{
    return entries_.end();
}

program_table::const_iterator program_table::end() const
{
    return entries_.end();
}
