#include "utilities.h"
#include "path.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

std::string Utility::create_log_directory()
{
    std::string s_path = get_program_directory();
    s_path += "log";
    s_path = Path::toNativeSeparators(s_path);

    if (!file_exists(s_path))
    {
#ifdef WIN32
        ::CreateDirectoryA(s_path.c_str(), nullptr);
        //SHCreateDirectoryExA(NULL, s_path.c_str(), NULL);
#else
        mkdir(s_path.c_str(), 0777);
#endif
    }
    return s_path;
}

std::string Utility::get_program_directory()
{
    std::string s_path;
#ifdef WIN32
    char sz_file_path[MAX_PATH] = { 0 };
    ::GetModuleFileNameA(NULL, sz_file_path, MAX_PATH);
    s_path = Path::simplifyPath(Path::fromNativeSeparators(sz_file_path).c_str());
    s_path = s_path.substr(0, s_path.rfind('/') + 1);
#else
    const int MAX_PATH = 0x100;
    char sz_file_path[MAX_PATH];
    int cnt = readlink("/proc/self/exe", sz_file_path, MAX_PATH);
    if (cnt < 0 || cnt >= MAX_PATH)
    {
        return s_path;
    }

    for (int i = cnt; i >= 0; --i)
    {
        if (sz_file_path[i] == '/')
        {
            sz_file_path[i + 1] = '\0';
            break;
        }
    }
    s_path = sz_file_path;
#endif

    return s_path;
}

bool Utility::file_exists(const std::string &path)
{
#ifdef WIN32
    bool b_exist = false;
    WIN32_FIND_DATAA wfd;
    HANDLE h_find = ::FindFirstFileA(path.c_str(), &wfd);
    if (INVALID_HANDLE_VALUE != h_find)
    {
        b_exist = true;
        ::FindClose(h_find);
    }
    return b_exist;
#else
    struct stat statinfo;
    int result = stat(path.c_str(), &statinfo);

    if (result < 0) { // Todo: should check errno == ENOENT?
        // File not found
        return false;
    }

    // Check if file is regular file
    if ((statinfo.st_mode & S_IFMT) == S_IFREG) {
        return true;
    }

    return false;
#endif
}

bool Utility::is_name_like_macro(const std::string &strName)
{
    bool b_has_upper = false;
    for (std::size_t ii = 0, n_len = strName.size(); ii < n_len; ++ii)
    {
        if (::islower(strName[ii]) || isdigit(strName[ii]))
        {
            return false;
        }
        if (!b_has_upper && ::isupper(strName[ii]))
        {
            b_has_upper = true;
        }
    }
    return b_has_upper;
}