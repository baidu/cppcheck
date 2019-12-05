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

#include "settings.h"

#include "valueflow.h"
////tsc
#include "tinyxml2.h"
#include "path.h"
#include "preprocessor.h"// Preprocessor
#include "utils.h"
#include <utility>
#include <fstream>
#include <set>
#include <algorithm>
#include <iostream>

bool Settings::_terminated;

Settings::Settings()
    : _enabled(0),
      debug(false),
      debugnormal(false),
      debugwarnings(false),
      dump(false),
      exceptionHandling(false),
      inconclusive(false),
      jointSuppressionReport(false),
      experimental(false),
      quiet(false),
      inlineSuppressions(false),
      verbose(false),
      force(false),
      relativePaths(false),
      xml(false), xml_version(2),
      _xml_conf_path(""),
      jobs(1),
      loadAverage(0),
      exitCode(0),
      showtime(SHOWTIME_NONE),
      preprocessOnly(false),
      maxConfigs(12),
      enforcedLang(None),
      reportProgress(false),
      checkConfiguration(false),
      checkLibrary(false)
      ////ANY_TYPE_T("T_2016_7_14_10_44_55_123_xxxx"),
      //fix THROW_EXCEPTION not expand
      ////returnKeyWord("return|break|continue|goto|throw|THROW_EXCEPTION"),
      ////_recordFuncinfo(false)
{

}

std::string Settings::addEnabled(const std::string &str)
{
    // Enable parameters may be comma separated...
    if (str.find(',') != std::string::npos) {
        std::string::size_type prevPos = 0;
        std::string::size_type pos = 0;
        while ((pos = str.find(',', pos)) != std::string::npos) {
            if (pos == prevPos)
                return std::string("cppcheck: --enable parameter is empty");
            const std::string errmsg(addEnabled(str.substr(prevPos, pos - prevPos)));
            if (!errmsg.empty())
                return errmsg;
            ++pos;
            prevPos = pos;
        }
        if (prevPos >= str.length())
            return std::string("cppcheck: --enable parameter is empty");
        return addEnabled(str.substr(prevPos));
    }

    if (str == "all") {
        _enabled |= WARNING | STYLE | PERFORMANCE | PORTABILITY | INFORMATION | UNUSED_FUNCTION | MISSING_INCLUDE;
    } else if (str == "warning") {
        _enabled |= WARNING;
    } else if (str == "style") {
        _enabled |= STYLE;
    } else if (str == "performance") {
        _enabled |= PERFORMANCE;
    } else if (str == "portability") {
        _enabled |= PORTABILITY;
    } else if (str == "information") {
        _enabled |= INFORMATION | MISSING_INCLUDE;
    } else if (str == "unusedFunction") {
        _enabled |= UNUSED_FUNCTION;
    } else if (str == "missingInclude") {
        _enabled |= MISSING_INCLUDE;
    }
#ifdef CHECK_INTERNAL
    else if (str == "internal") {
        _enabled |= INTERNAL;
    }
#endif
    else {
        if (str.empty())
            return std::string("cppcheck: --enable parameter is empty");
        else
            return std::string("cppcheck: there is no --enable parameter with the name '" + str + "'");
    }

    return std::string();
}

bool Settings::isEnabled(Severity::SeverityType severity) const
{
    switch (severity) {
    case Severity::none:
        return true;
    case Severity::error:
        return true;
    case Severity::warning:
        return isEnabled(WARNING);
    case Severity::style:
        return isEnabled(STYLE);
    case Severity::performance:
        return isEnabled(PERFORMANCE);
    case Severity::portability:
        return isEnabled(PORTABILITY);
    case Severity::information:
        return isEnabled(INFORMATION);
    case Severity::debug:
        return false;
    default:
        return false;
    }
}

bool Settings::isEnabled(const ValueFlow::Value *value, bool inconclusiveCheck) const
{
    if (!isEnabled(Settings::WARNING) && (value->condition || value->defaultArg))
        return false;
    if (!inconclusive && (inconclusiveCheck || value->isInconclusive()))
        return false;
    return true;
}

bool Settings::load_custom_cfg_xml(const std::string &filePath, const char szExeName[])
{
    OpenCodeTrace = false;//init false
    //// tsc
    ////OpenAutofilter = false;//init false

    //load types
    std::string base_type[] = {
        "int",
        "long",
        "short",
        "float",
        "double",
        "char",
        "bool",
        "size_t",
        "int8_t",
        "int16_t",
        "int32_t",
        "int64_t",
        "uint8_t",
        "uint16_t",
        "uint32_t",
        "uint64_t",
        "int_fast8_t",
        "int_fast16_t",
        "int_fast32_t",
        "int_fast64_t",
        "int_least8_t",
        "int_least16_t",
        "int_least32_t",
        "int_least64_t",
        "uint_fast8_t",
        "uint_fast16_t",
        "uint_fast32_t",
        "uint_fast64_t",
        "uint_least8_t",
        "uint_least16_t",
        "uint_least32_t",
        "uint_least64_t"
    };
    for (int index = 0; index < 32; ++index)
    {
        _equalTypeForFuncSig.insert(base_type[index]);
        _equalTypeForFuncSig.insert(std::string("long ") + base_type[index]);
        _equalTypeForFuncSig.insert(std::string("unsigned ") + base_type[index]);
        _equalTypeForFuncSig.insert(std::string("unsigned long ") + base_type[index]);
    }

    _keywordBeforeFunction.insert("extern");
    _keywordBeforeFunction.insert("static");
    _keywordBeforeFunction.insert("inline");
    _keywordBeforeFunction.insert("constexpr");

    _nonPtrType.insert("Int32");
    _nonPtrType.insert("UInt32");
    _nonPtrType.insert("time_t");

    //load from file
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError error = doc.LoadFile(filePath.c_str());

    if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
    {
        //try abs path
#ifdef CFGDIR
        const std::string cfgfolder(CFGDIR);
#else
        if (!szExeName)
        {
            std::cout << "Failed to load setting file 'cfg.xml'. File not found" << std::endl;
            return false;
        }
        const std::string cfgfolder(Path::fromNativeSeparators(Path::getPathFromFilename(szExeName))
            + "cfg");
#endif
        const char *sep = (!cfgfolder.empty() &&
            cfgfolder[cfgfolder.size() - 1U] == '/' ? "" : "/");

        const std::string filename(cfgfolder + sep + "cfg.xml");
        error = doc.LoadFile(filename.c_str());

        if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
        {
            const std::string filename2(cfgfolder + sep + "cfg_setup.xml");
            error = doc.LoadFile(filename2.c_str());
        }
        if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
        {
            std::cout << "Failed to load setting file 'cfg.xml'. File not found" << std::endl;
            return false;
        }
    }

    const tinyxml2::XMLElement * const rootnode = doc.FirstChildElement();

    if (rootnode == nullptr)
    {
        std::cout << "cfg.xml format illegal" << std::endl;
        return false;
    }

    if (strcmp(rootnode->Name(), "def") != 0)
    {
        std::cout << "cfg.xml format not supported" << std::endl;
        return false;
    }

    // version check like other .cfg files
    const char* format_string = rootnode->Attribute("format");
    int format = 1;
    if (format_string)
    {
        format = atoi(format_string);
    }

    if (format > 2 || format <= 0)
    {
        std::cout << "cfg.xml format version not supported" << std::endl;
        return false;
    }

    for (const tinyxml2::XMLElement *node = rootnode->FirstChildElement();
        node; node = node->NextSiblingElement())
    {
        //ignore unknown tag
        if (!node->Name() || 0 != strcmp(node->Name(), "section"))
        {
            continue;
        }
        const char * pNodeName = node->Attribute("name");
        if (!pNodeName)
        {
            continue;
        }
        if (0 == strcmp(pNodeName, "FunctionNotReturnNull"))
        {
            for (const tinyxml2::XMLElement *func = node->FirstChildElement();
                func; func = func->NextSiblingElement())
            {
                //only record known tag
                if (func->Name() && strcmp(func->Name(), "function") == 0)
                {
                    const char *pAttr = func->Attribute("name");
                    std::pair<std::string, std::vector<std::string> > sig;
                    std::string function_name;
                    if (pAttr && parse_func_info(function_name, sig, pAttr))
                    {
                        _functionNotRetNull.insert(std::make_pair(function_name, sig));
                    }
                }
            }
        }/* tsc
         else if (0 == strcmp(pNodeName, "PathToIgnore"))
         {
         for (const tinyxml2::XMLElement *path = node->FirstChildElement(); path; path = path->NextSiblingElement())
         {
         //only record known tag
         if (path->Name() && strcmp(path->Name(), "path") == 0)
         {
         const char *pAttr = path->Attribute("name");
         if (pAttr)
         {
         _pathToIgnore.push_back(path->Attribute("name"));
         }
         }
         }
         }*/
        else if (0 == strcmp(pNodeName, "JumpCode"))
        {
            std::set<int> param_index;
            for (const tinyxml2::XMLElement *jmp = node->FirstChildElement();
                jmp; jmp = jmp->NextSiblingElement())
            {
                //only record known tag
                if (jmp->Name() && strcmp(jmp->Name(), "jumpcode") == 0)
                {
                    for (const tinyxml2::XMLElement *arg = jmp->FirstChildElement();
                        arg; arg = arg->NextSiblingElement())
                    {
                        if (arg->Name() && strcmp(arg->Name(), "arg") == 0)
                        {
                            const char *pIndex = arg->Attribute("index");
                            if (pIndex)
                            {
                                param_index.insert(atoi(pIndex));
                            }
                        }
                    }
                }
                const char *pName = jmp->Attribute("name");
                if (pName)
                {
                    _jumpCode[pName] = param_index;
                    //clear paramIndex for reuse
                    param_index.clear();
                }
            }
        }/* tsc
         else if (0 == strcmp(pNodeName, "IgnoreFileExtension"))
         {
         for (const tinyxml2::XMLElement *jmp = node->FirstChildElement(); jmp; jmp = jmp->NextSiblingElement())
         {
         //only record known tag
         if (jmp->Name() && strcmp(jmp->Name(), "ext") == 0)
         {
         const char *pName = jmp->Attribute("name");

         if (pName)
         {
         std::string strName = pName;
         std::transform(strName.begin(), strName.end(), strName.begin(), tolower);
         _extensionToIgnore.push_back(strName);
         }
         }
         }
         }*/
        else if (0 == strcmp(pNodeName, "Checks"))
        {
            for (const tinyxml2::XMLElement *id2 = node->FirstChildElement();
                id2; id2 = id2->NextSiblingElement())
            {
                if (id2->Name() && (0 == strcmp(id2->Name(), "id")))
                {
                    bool b_open_id = id2->BoolAttribute("value");
                    const char* szId = id2->Attribute("name");
                    if (szId)
                    {
                        std::map < std::string, bool >& subid_set = _openedChecks[szId];

                        for (const tinyxml2::XMLElement *subid = id2->FirstChildElement();
                            subid; subid = subid->NextSiblingElement())
                        {
                            if (subid->Name() && (0 == strcmp(subid->Name(), "subid")))
                            {
                                const char* szCheck = subid->Attribute("name");
                                if (szCheck)
                                {
                                    bool b_open = subid->BoolAttribute("value");
                                    subid_set[szCheck] = b_open_id ? b_open : false;

                                    const char* szSeverity = subid->Attribute("severity");
                                    if (szSeverity) {
                                        _checkSeverity[szCheck] = szSeverity;
                                    }
                                    else
                                        _checkSeverity[szCheck] = "Warning";

                                }
                            }
                        }
                    }

                }
            }
        }
        else if (0 == strcmp(pNodeName, "PerformanceCfg"))
        {
            for (const tinyxml2::XMLElement *id2 = node->FirstChildElement();
                id2; id2 = id2->NextSiblingElement())
            {
                if (id2->Name() && (0 == strcmp(id2->Name(), "entry")))
                {
                    const char* szEntry = id2->Attribute("name");
                    if (szEntry)
                    {
                        if (0 == strcmp(szEntry, "large_token_count"))
                        {
                            int i_value = id2->IntAttribute("value");
                            _big_file_token_size = i_value;
                        }
                        else if (0 == strcmp(szEntry, "large_header_size"))
                        {
                            int i_value = id2->IntAttribute("value");
                            _big_header_file_size = i_value;
                        }
                        else if (0 == strcmp(szEntry, "large_includes"))
                        {
                            int i_value = id2->IntAttribute("value");
                            _large_includes = i_value;
                        }
                    }
                }
            }
        }
        else if (0 == strcmp(pNodeName, "NonReferenceType"))
        {
            for (const tinyxml2::XMLElement *rType = node->FirstChildElement();
                rType; rType = rType->NextSiblingElement())
            {
                //only record known tag
                if (rType->Name() && strcmp(rType->Name(), "typename") == 0)
                {
                    const char *pAttr = rType->Attribute("name");
                    if (pAttr)
                    {
                        std::string typestr = rType->Attribute("name");
                        transform(typestr.begin(), typestr.end(), typestr.begin(), ::tolower);
                        _NonReferenceType.insert(typestr);
                    }
                }
            }
        }
        else if (0 == strcmp(pNodeName, "SelfMemMalloc"))
        {
            for (const tinyxml2::XMLElement *rType = node->FirstChildElement();
                rType; rType = rType->NextSiblingElement())
            {
                //only record known tag
                if (rType->Name() && strcmp(rType->Name(), "function") == 0)
                {
                    const char *pAttr = rType->Attribute("customSelfMalloc");
                    const char *pAttr2 = rType->Attribute("customSelfDelloc");
                    if (pAttr && pAttr2)
                    {
                        std::string self_malloc_str = rType->Attribute("customSelfMalloc");
                        std::string self_dealloc_str = rType->Attribute("customSelfDelloc");

                        _customSelfMalloc.push_back(self_malloc_str);
                        _customSelfDelloc.push_back(self_dealloc_str);
                    }
                }
            }
        }
        else if (0 == strcmp(pNodeName, "CodeTrace"))
        {
            OpenCodeTrace = node->BoolAttribute("value");
        }
        //// tsc
        /*else if (0 == strcmp(pNodeName, "Autofilter"))
        {
        OpenAutofilter = node->BoolAttribute("value");
        }*/
        else if (0 == strcmp(pNodeName, "HeaderExtension"))
        {
            for (const tinyxml2::XMLElement *rType = node->FirstChildElement(); rType;
                rType = rType->NextSiblingElement())
            {
                //only record known tag
                if (rType->Name() && strcmp(rType->Name(), "ext") == 0)
                {
                    const char *pAttr = rType->Attribute("name");
                    if (pAttr && pAttr[0] != '\0')
                    {
                        _headerExtension.insert(pAttr);
                    }
                }
            }
        }
    }

    return true;
}

////tsc
const void Settings::trim(std::string &str) const
{
    const std::size_t start = str.find_first_not_of(' ');
    if (start == std::string::npos)
    {
        str.clear();
        return;
    }

    const std::size_t end = str.find_last_not_of(' ');
    if (start == std::string::npos)
    {
        str.clear();
        return;
    }
    str = str.substr(start, end - start + 1);
}

void Settings::split(std::vector<std::string> &tokens, const std::string &str,
    const std::string &delimiters) const
{
    // Skip delimiters at beginning.
    std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, last_pos);

    while (std::string::npos != pos || std::string::npos != last_pos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(last_pos, pos - last_pos));
        // Skip delimiters.  Note the "not_of"
        last_pos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, last_pos);
    }
}

bool Settings::parse_func_info(std::string& functionname, std::pair<std::string,
    std::vector<std::string> >& sig, const std::string &funcinfo)
{
    if (funcinfo.empty())
    {
        return false;
    }

    std::string::size_type pos_functionname = 0;

    std::string funcinfo_bak = funcinfo;
    std::string classname_parse = "";
    std::string functionname_parse = "";
    std::vector<std::string> class_list;
    std::vector<std::string> classname_list;
    std::vector<std::string> funcinfo_list;
    std::vector<std::string> funcname_list;
    std::string str_head;

    if ((pos_functionname = funcinfo.find("(")) != std::string::npos)
    {
        str_head = funcinfo.substr(0, pos_functionname);
    }
    //only function name
    else
    {
        str_head = funcinfo;
    }
    //split function name
    trim(str_head);
    if (str_head.empty())
    {
        return false;
    }
    //split string by "::"
    split(class_list, str_head, "::");
    if (class_list.size() >= 2) {
        classname_parse = class_list[class_list.size() - 2];
        trim(classname_parse);
        split(classname_list, classname_parse, " ");
        if (classname_list.size() >= 2 && classname_list[classname_list.size() - 1] == ">") {
            std::string tmp;
            bool flag_template = false;
            for (std::size_t i = 0; i < classname_list.size(); i++) {
                if (classname_list[i] == "<" && i > 0) {
                    flag_template = true;
                    tmp += classname_list[i - 1];
                }
                if (flag_template) {
                    tmp += classname_list[i];
                }
            }
            classname_parse = tmp;
        }
        else if (!classname_list.empty()) {
            classname_parse = classname_list[classname_list.size() - 1];
        }

        functionname_parse = class_list[class_list.size() - 1];
        trim(functionname_parse);

    }
    else //no class name
    {
        size_t n_pos = str_head.find('<');
        if (n_pos != std::string::npos)
        {
            functionname_parse = str_head.substr(0, n_pos);
        }
        else
        {
            functionname_parse = str_head;
        }
    }
    //only function name
    if (pos_functionname == std::string::npos)
    {
        sig.first = classname_parse;
        //////sig.second.push_back(ANY_TYPE_T);
        functionname = functionname_parse;
        return true;
    }

    std::string::size_type params_start = 0;
    std::string::size_type params_end = 0;
    std::string params_line;

    if ((params_start = funcinfo.find("(")) != std::string::npos) {
        if ((params_end = funcinfo.rfind(")")) != std::string::npos && params_start < params_end) {
            params_line = funcinfo.substr(params_start + 1, params_end - params_start - 1);
            trim(params_line);
        }
    }

    if (params_line != "") {
        std::string::size_type pos;
        std::string pattern(",");

        params_line += pattern;
        bool find_map = false;

        if ((params_line.find("map")) != std::string::npos) {
            find_map = true;
        }

        unsigned int size = params_line.size();
        for (std::size_t i = 0; i < size; i++) {
            pos = params_line.find(pattern, i);
            if (pos < size) {
                int count = 0;
                if (find_map) {
                    for (unsigned int j = pos; j < size; j++) {
                        if (params_line[j] == '<') {
                            count++;
                        }
                        else if (params_line[j] == '>') {
                            count--;
                        }
                    }
                }
                if (count < 0) {
                    pos = params_line.find(pattern, pos + 1);
                    if (!(pos < size)) {
                        continue;
                    }
                }
                std::string s = params_line.substr(i, pos - i);
                trim(s);
                std::string::size_type pos_param_name;
                if ((pos_param_name = s.rfind(" ")) != std::string::npos) {
                    char c2 = s[pos_param_name + 1];
                    s = s.substr(0, pos_param_name);
                    trim(s);
                    char c = s[s.length() - 1];
                    if (c2 == '&' || c2 == '*') {
                        s += c2;
                    }
                    else if (c == '&' || c == '*') {
                        s = s.substr(0, s.length() - 1);
                        trim(s);
                        s += c;
                    }

                }
                if (_equalTypeForFuncSig.find(s) != _equalTypeForFuncSig.end())
                {
                    sig.second.push_back("int");
                }
                else
                {
                    sig.second.push_back(s);
                }
                i = pos;
            }
        }
    }

    sig.first = classname_parse;
    functionname = functionname_parse;
    return true;
}