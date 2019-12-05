#ifndef configH
#define configH

#ifdef _WIN32
#  ifdef CPPCHECKLIB_EXPORT
#    define CPPCHECKLIB __declspec(dllexport)
#  elif defined(CPPCHECKLIB_IMPORT)
#    define CPPCHECKLIB __declspec(dllimport)
#  else
#    define CPPCHECKLIB
#  endif
#else
#  define CPPCHECKLIB
#endif

// MS Visual C++ memory leak debug tracing
#if defined(_MSC_VER) && defined(_DEBUG)
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#endif

#include <string>
#include <algorithm>
#include <vector>
#include <set>

////tsc
#if (defined(__GNUC__) || defined(__sun)) && !defined(__MINGW32__)
#define TSC_THREADING_MODEL_NOT_WIN
#elif defined(_WIN32)
#define TSC_THREADING_MODEL_WIN
//#include <windows.h>
#endif
#ifdef TSC_THREADING_MODEL_WIN

#define TSC_THREAD              HANDLE
#define TSC_LOCK                CRITICAL_SECTION
#define TSC_LOCK_ENTER(lock)    EnterCriticalSection(lock)
#define TSC_LOCK_LEAVE(lock)    LeaveCriticalSection(lock)
#define TSC_LOCK_INIT(lock)     InitializeCriticalSection(lock)
#define TSC_LOCK_DELETE(lock)   DeleteCriticalSection(lock)

#define TSC_MAX                 max
#define TSC_MIN                 min

#define PATH_SEP	'\\'

#else

#define TSC_THREAD              pthread_t
#define TSC_LOCK                pthread_mutex_t
#define TSC_LOCK_ENTER(lock)    pthread_mutex_lock(lock)
#define TSC_LOCK_LEAVE(lock)    pthread_mutex_unlock(lock)
#define TSC_LOCK_INIT(lock)     pthread_mutex_init(lock, nullptr)
#define TSC_LOCK_DELETE(lock)   pthread_mutex_destroy(lock)

#define TSC_MAX                 std::max
#define TSC_MIN                 std::min

#define PATH_SEP	'/'

#endif

static const std::string emptyString;
////tsc
#define SAFE_DELETE(p) { if (p) { delete p; p = nullptr; } }
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] p; p = nullptr; } }
#define TSC2_DUMP_TYPE_TREE

class CommonFile {

public:

    void string_split(const std::string& str, std::vector<std::string>& tokens,
        const std::string& delimiters) {
        // Skip delimiters at beginning.
        std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
        // Find first "non-delimiter".
        std::string::size_type pos = str.find_first_of(delimiters, last_pos);
        while (std::string::npos != pos || std::string::npos != last_pos) {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(last_pos, pos - last_pos));
            // Skip delimiters.  Note the "not_of"
            last_pos = str.find_first_not_of(delimiters, pos);
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, last_pos);
        }
    }

    void string_split_uniq(const std::string& str, std::set<std::string>& tokens,
        const std::string& delimiters) {
        // Skip delimiters at beginning.
        std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
        // Find first "non-delimiter".
        std::string::size_type pos = str.find_first_of(delimiters, last_pos);
        while (std::string::npos != pos || std::string::npos != last_pos) {
            // Found a token, add it to the vector.
            tokens.insert(str.substr(last_pos, pos - last_pos));
            // Skip delimiters.  Note the "not_of"
            last_pos = str.find_first_not_of(delimiters, pos);
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, last_pos);
        }
    }

    std::string str_uniq(const std::string& input, size_t& count) {
        std::set<std::string> temp_set;
        std::string temp_str = "";
        if (input != "") {
            string_split_uniq(input, temp_set, "|");
            for (std::set<std::string>::iterator it = temp_set.begin();
                it != temp_set.end(); ++it) {
                std::string temp = *it;
                temp_str = temp_str.append(temp).append("|");
            }
            count = temp_set.size();
        }
        return temp_str;
    }

    bool find_in_vector(std::vector<std::string>& vec, const std::string& value) {
        std::vector<std::string>::iterator it = std::find(vec.begin(), vec.end(), value);
        if (it == vec.end()) {
            return false;
        }
        else {
            return true;
        }
    }
};

#endif // configH
