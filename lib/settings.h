/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2017 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#ifndef settingsH
#define settingsH
//---------------------------------------------------------------------------

#include "config.h"
#include "errorlogger.h"
#include "importproject.h"
#include "library.h"
#include "platform.h"
#include "standards.h"
#include "suppressions.h"
#include "timer.h"

#include <list>
#include <set>
#include <string>
#include <vector>

namespace ValueFlow {
    class Value;
}

/// @addtogroup Core
/// @{

/**
 * @brief This is just a container for general settings so that we don't need
 * to pass individual values to functions or constructors now or in the
 * future when we might have even more detailed settings.
 */
class CPPCHECKLIB Settings : public cppcheck::Platform {
public:
    enum EnabledGroup {
        WARNING = 0x1,
        STYLE = 0x2,
        PERFORMANCE = 0x4,
        PORTABILITY = 0x8,
        INFORMATION = 0x10,
        UNUSED_FUNCTION = 0x20,
        MISSING_INCLUDE = 0x40,
        INTERNAL = 0x80
    };

private:
    /** @brief enable extra checks by id */
    int _enabled;

    /** @brief terminate checking */
    static bool _terminated;
    ////tsc
    ////static Settings *_instance;
    std::map<std::string, std::set<int> > _jumpCode;
    
    std::map<std::string, std::string> _checkSeverity;
    std::map<std::string, std::map<std::string, bool> > _openedChecks;
    bool parse_func_info(std::string&, std::pair<std::string, std::vector<std::string> >& funcNotRetNull, const std::string &funcinfo);
    const void trim(std::string &str) const;
    void split(std::vector<std::string> &out, const std::string &input, const std::string &delim) const;
public:
    Settings();
    ////Settings & operator=(Settings&) { return *this; };
    /** @brief --cppcheck-build-dir */
    std::string buildDir;

    /** @brief Is --debug given? */
    bool debug;

    /** @brief Is --debug-normal given? */
    bool debugnormal;

    /** @brief Is --debug-warnings given? */
    bool debugwarnings;

    /** @brief Is --dump given? */
    bool dump;
    std::string dumpFile;

    /** @brief Is --exception-handling given */
    bool exceptionHandling;

    /** @brief Inconclusive checks */
    bool inconclusive;

    /** @brief Collect unmatched suppressions in one run.
      * This delays the reporting until all files are checked.
      * It is needed by checks that analyse the whole code base. */
    bool jointSuppressionReport;

    /**
     * When this flag is false (default) then experimental
     * heuristics and checks are disabled.
     *
     * It should not be possible to enable this from any client.
     */
    bool experimental;

    /** @brief Is --quiet given? */
    bool quiet;

    /** @brief Is --inline-suppr given? */
    bool inlineSuppressions;

    /** @brief Is --verbose given? */
    bool verbose;

    /** @brief Request termination of checking */
    static void terminate(bool t = true) {
        Settings::_terminated = t;
    }

    /** @brief termination requested? */
    static bool terminated() {
        return Settings::_terminated;
    }

    /** @brief Force checking the files with "too many" configurations (--force). */
    bool force;

    /** @brief Use relative paths in output. */
    bool relativePaths;

    /** @brief Paths used as base for conversion to relative paths. */
    std::vector<std::string> basePaths;

    /** @brief write results (--output-file=&lt;file&gt;) */
    std::string outputFile;

    /** @brief plist output (--plist-output=&lt;dir&gt;) */
    std::string plistOutput;

    /** @brief write XML results (--xml) */
    bool xml;

    /** @brief XML version (--xml-version=..) */
    int xml_version;

    std::string _xml_conf_path;

    /** @brief How many processes/threads should do checking at the same
        time. Default is 1. (-j N) */
    unsigned int jobs;

    /** @brief Load average value */
    unsigned int loadAverage;

    /** @brief If errors are found, this value is returned from main().
        Default value is 0. */
    int exitCode;

    /** @brief The output format in which the errors are printed in text mode,
        e.g. "{severity} {file}:{line} {message} {id}" */
    std::string outputFormat;

    /** @brief show timing information (--showtime=file|summary|top5) */
    SHOWTIME_MODES showtime;

    /** @brief Using -E for debugging purposes */
    bool preprocessOnly;

    /** @brief List of include paths, e.g. "my/includes/" which should be used
        for finding include files inside source files. (-I) */
    std::list<std::string> includePaths;

    /** @brief Maximum number of configurations to check before bailing.
        Default is 12. (--max-configs=N) */
    unsigned int maxConfigs;
    bool CheckIfJumpCode(const std::string& str) const { return _jumpCode.find(str) != _jumpCode.end(); }
    /**
     * @brief Returns true if given id is in the list of
     * enabled extra checks (--enable)
     * @param group group to be enabled
     * @return true if the check is enabled.
     */
    bool isEnabled(EnabledGroup group) const {
        return (_enabled & group) == group;
    }

    /**
    * @brief Returns true if given severity is enabled
    * @return true if the check is enabled.
    */
    bool isEnabled(Severity::SeverityType severity) const;

    /**
    * @brief Returns true if given value can be shown
    * @return true if the value can be shown
    */
    bool isEnabled(const ValueFlow::Value *value, bool inconclusiveCheck=false) const;

    /**
     * @brief Enable extra checks by id. See isEnabled()
     * @param str single id or list of id values to be enabled
     * or empty string to enable all. e.g. "style,possibleError"
     * @return error message. empty upon success
     */
    std::string addEnabled(const std::string &str);

    /**
     * @brief Disables all severities, except from error.
     */
    void clearEnabled() {
        _enabled = 0;
    }

    enum Language {
        None, C, CPP
    };

    /** @brief Name of the language that is enforced. Empty per default. */
    Language enforcedLang;

    /** @brief suppress message (--suppressions) */
    Suppressions nomsg;

    /** @brief suppress exitcode */
    Suppressions nofail;

    /** @brief defines given by the user */
    std::string userDefines;

    /** @brief undefines given by the user */
    std::set<std::string> userUndefs;

    /** @brief forced includes given by the user */
    std::list<std::string> userIncludes;

    /** @brief include paths excluded from checking the configuration */
    std::set<std::string> configExcludePaths;


    /** @brief --report-progress */
    bool reportProgress;

    /** Library (--library) */
    Library library;

    /** Rule */
    class CPPCHECKLIB Rule {
    public:
        Rule()
            : tokenlist("simple")         // use simple tokenlist
            , id("rule")                  // default id
            , severity(Severity::style) { // default severity
        }

        std::string tokenlist;
        std::string pattern;
        std::string id;
        std::string summary;
        Severity::SeverityType severity;
    };

    /**
     * @brief Extra rules
     */
    std::list<Rule> rules;

    /** Is the 'configuration checking' wanted? */
    bool checkConfiguration;

    /** Check for incomplete info in library files? */
    bool checkLibrary;

    /** Struct contains standards settings */
    Standards standards;

    ImportProject project;

    /**
     * @brief return true if a included file is to be excluded in Preprocessor::getConfigs
     * @return true for the file to be excluded.
     */
    bool configurationExcluded(const std::string &file) const {
        for (std::set<std::string>::const_iterator i=configExcludePaths.begin(); i!=configExcludePaths.end(); ++i) {
            if (file.length()>=i->length() && file.compare(0,i->length(),*i)==0) {
                return true;
            }
        }
        return false;
    }
    bool load_custom_cfg_xml(const std::string &strPath, const char szExeName[]);
    ////const std::string ANY_TYPE_T;
    ////const char* returnKeyWord;
    ////bool _recordFuncinfo;
    std::set<std::string> _equalTypeForFuncSig;
    std::set<std::string> _nonPtrType;
    std::set<std::string> _keywordBeforeFunction;
    bool OpenCodeTrace;
    std::multimap<std::string, std::pair<std::string, std::vector<std::string> > >  _functionNotRetNull;
    int _big_file_token_size;
    int _big_header_file_size;
    int _large_includes;
    std::set<std::string>  _NonReferenceType;
    std::vector<std::string>  _customSelfMalloc;
    std::vector<std::string>  _customSelfDelloc;
    std::set<std::string> _headerExtension;
};

/// @}
//---------------------------------------------------------------------------
#endif // settingsH
