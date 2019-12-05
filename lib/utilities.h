#ifndef BCACPP_UTILITIES_H
#define BCACPP_UTILITIES_H

#include <string>

class Utility
{
public:
    static std::string create_log_directory();
    static bool is_name_like_macro(const std::string &strName);
private:
    static std::string get_program_directory();

    static bool file_exists(const std::string &path);
};

#endif // BCACPP_UTILITIES_H
