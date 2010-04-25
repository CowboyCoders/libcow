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

bool program_table::load_from_file(const std::string& file_name)
{
    std::ifstream fs(file_name.c_str(), std::ios_base::in);
    if (!fs) {
        BOOST_LOG_TRIVIAL(error) << "program_table: Error opening file \"" << file_name << "\".";
        return false;
    }

    fs.seekg(0, std::ios_base::end);
    size_t length = static_cast<size_t>(fs.tellg());
    fs.seekg(0);

    // Allocate and read the whole file in one read
    char* buffer = new char[length];
    fs.read(buffer, length);

    // Copy buffer to a string
    std::string s(buffer);
    delete [] buffer;

    return load_from_string(s);    
}

bool program_table::load_from_http(const std::string& url)
{
    std::string s = libcow::http_get_as_string(url);
    BOOST_LOG_TRIVIAL(debug) << "program_table: Downloaded program table with size:" << s.length() << ".";
    return load_from_string(s);
}

bool program_table::load_from_string(const std::string& s)
{
    // Clear old program table entries if any
    clear();

    TiXmlDocument doc;
    doc.Parse(s.c_str());

    if (doc.Error()) {
        BOOST_LOG_TRIVIAL(error) << "program_table: Parse error:" << doc.ErrorDesc();
        return false;
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
	
    return true;
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

const libcow::program_info& program_table::at(size_t index) const
{
    return entries_[index];
}

libcow::program_info& program_table::at(size_t index)
{
    return entries_[index];
}

const libcow::program_info& program_table::operator[](size_t index) const
{
    return entries_[index];
}

libcow::program_info& program_table::operator[](size_t index)
{
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
