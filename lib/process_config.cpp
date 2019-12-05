#include "process_config.h"
#include <cstdlib>
#include <map>

ProcessConfig ProcessConfig::_s_instance;
std::string ProcessConfig::_s_dir;
static std::map<std::string, std::string>* s_configurations = nullptr;

ProcessConfig::ProcessConfig() {

}

ProcessConfig::~ProcessConfig(){
}

bool ProcessConfig::load_xml_conf() {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError error = doc.LoadFile((_s_dir.append("check_list.xml")).c_str());
    if (error != tinyxml2::XML_SUCCESS) {
        return false;
    }
    const tinyxml2::XMLElement * const rootNode = doc.FirstChildElement();
    if (rootNode == nullptr) {
        return false;
    }

    const char* root_node_value = rootNode->Value();
    if (!root_node_value) {
        return false;
    }

    const tinyxml2::XMLElement *child_element = rootNode->FirstChildElement("constinfo");

    if (child_element && child_element->Attribute("isopen")) {
        const char* str = child_element->Attribute("isopen");
        add_configuration(std::string("constinfo"), std::string(str));
    }
    const tinyxml2::XMLElement *sub_element = NULL;
    if (child_element) {
        sub_element = child_element->FirstChildElement("subid");

        for (; sub_element; sub_element = sub_element->NextSiblingElement()) {
            root_node_value = sub_element->Attribute("name");
            if (strcmp(root_node_value, "EXIT") == 0 && sub_element->Attribute("value")) {
                const char* str = sub_element->Attribute("value");
                add_configuration(std::string("EXIT"), std::string(str));
                continue;
            }
            if (strcmp(root_node_value, "NULL") == 0 && sub_element->Attribute("value")) {
                const char* str = sub_element->Attribute("value");
                add_configuration(std::string("NULL"), std::string(str));
                continue;
            }
            if (strcmp(root_node_value, "RETURN_VALUE") == 0 && sub_element->Attribute("value")) {
                const char* str = sub_element->Attribute("value");
                add_configuration(std::string("RETURN_VALUE"), std::string(str));
                continue;
            }
        }
    }
    return true;
}

void ProcessConfig::add_configuration(std::string key, std::string value) {
    if (s_configurations == nullptr) {
        s_configurations = new std::map<std::string, std::string>();
    }
    s_configurations->operator[](key) = value;
}

bool ProcessConfig::has_key(std::string key) {
    return s_configurations != nullptr &&
        s_configurations->find(key) != s_configurations->end();
}

int ProcessConfig::key_to_int(std::string key, int defaultValue) {
    return has_key(key) ? atoi(value_of_key(key).c_str()) : defaultValue;
}

std::string ProcessConfig::value_of_key(std::string key) {
    return s_configurations->operator[](key);
}

void ProcessConfig::delete_configurations() {
    delete[]s_configurations;
    s_configurations = nullptr;
}