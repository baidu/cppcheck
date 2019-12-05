#ifndef PROCESS_CONFIG__H
#define PROCESS_CONFIG__H

#include <tinyxml2.h>
#include <string>

class ProcessConfig {

public:

    ProcessConfig();
    ~ProcessConfig();

    static void load_xml_conf_dir(const std::string &dir) {
        if (!dir.empty()) {
            _s_dir = dir;
        }
        _s_instance.load_xml_conf();
    }

    bool load_xml_conf();
    static bool has_key(std::string key);
    static int key_to_int(std::string key, int defaultValue = 0);
    static void add_configuration(std::string key, std::string value);
    static std::string value_of_key(std::string key);
    static void delete_configurations();
private:
    static std::string _s_dir;
    static ProcessConfig _s_instance;
};
#endif // PROCESS_CONFIG__H