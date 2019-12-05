#ifndef BCACPP_GLOBALTOKENIZER_H
#define BCACPP_GLOBALTOKENIZER_H

#include <map>
#include <list>
#include "config.h"
#include "globalsymboldatabase.h"

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

class SymbolDatabase;
class Scope;
class Variable;

//from bebezhang 20140623
struct  FUNCINFO
{
    std::string filename;
    std::string functionname;
    int startline;
    int endline;
};

struct SPack1Scope 
{
    unsigned StartLine;
    unsigned EndLine;

    SPack1Scope(unsigned start, unsigned end) : StartLine(start), EndLine(end) { }
    SPack1Scope() : StartLine(0), EndLine(0) { }

    bool operator <(const SPack1Scope& scope) const
    {
        return StartLine < scope.StartLine;
    }
};

typedef std::map<std::string, std::set<SPack1Scope> >::iterator            Pack1MapI;
typedef std::map<std::string, std::set<SPack1Scope> >::const_iterator    Pack1MapCI;

class CGlobalTokenizeData
{
public:
    CGlobalTokenizeData();
    ~CGlobalTokenizeData();

    void dump(const char* szPath) const;
    
    void merge(const CGlobalTokenizeData& threadData);
    void RelateTypeInfo();
    void InitScopes(const SymbolDatabase* symbolDB);
    const gt::CGlobalScope* GetData() const { return m_data; }
    
    void record_func_info(const Tokenizer* tokenizer, const std::string &strFileName);
    void record_info_for_lua(const Tokenizer* tokenizer, const std::string &strFileName);

    void dump_funcinfo();
    void merge_lua_info_to_one_data(std::set<std::string>& m_oneLuaData,
        std::set<std::string> &luaExportClass);

    void add_pack_1_scope(const std::string& filename, unsigned start, unsigned end);
    std::map<std::string, std::set<SPack1Scope> >& GetPack1Scope();
    void record_risk_types(const Tokenizer* tokenizer);

    bool is_risk_type(const Variable& var, const Scope* scope);
    const std::set<std::string>& get_risk_types() { return m_riskTypes; }

    void add_lua_info(const std::string &info) { m_InfoForLua.insert(info); }
    std::set<std::string>& get_lua_info() { return m_InfoForLua; }

    void add_export_class(const std::string &str) { m_luaExportClass.insert(str); }
    std::set<std::string>& get_export_class() { return m_luaExportClass; }
    bool recored_export_class() const { return m_bRecoredExportClass; }
    void recored_export_class(bool b) { m_bRecoredExportClass = b; }
private:
    void sync_scopes(const Scope* scope, gt::CScope* gtScope, std::set<const Scope*>& added);
    void record_scope(gt::CScope* scope);

    gt::CScope* FindType(const std::vector<std::string>& tl, gt::CScope* cs);

public:
    typedef std::multimap<std::string, gt::CScope*> SM;
    typedef std::multimap<std::string, gt::CScope*>::iterator SMI;
    typedef std::multimap<std::string, gt::CScope*>::const_iterator SMCI;
private:

    gt::CGlobalScope* m_data;
    std::multimap<std::string, gt::CScope*> m_scopeMap;
    std::map<const Scope*, gt::CScope*> m_scopeCache;

    std::list<FUNCINFO> m_funcInfoList;
    std::set<std::string> m_InfoForLua;
    /////[LUA.EXPORT.CLASS]
    std::set<std::string> m_luaExportClass;
    bool m_bRecoredExportClass;
    std::map<std::string, std::set<SPack1Scope> > m_pack1Scopes;
    std::set<std::string> m_riskTypes;

    static std::set<std::string> s_stdTypes;
};

class CPPCHECKLIB CGlobalTokenizer
{
public:
    static CGlobalTokenizer* Instance();
    static CGlobalTokenizer* s_instance;

    static void uninitialize();

    CGlobalTokenizeData* get_global_data(void* pKey);
    
    void merge(bool dump = false);
 
    gt::CFuncData::RetNullFlag check_func_return_null(const Token* tokFunc);

    bool check_func_exit(const Token*tokFunc) const;

    bool check_if_match_signature(const Token *tokFunc, 
        const std::multimap<std::string, std::pair<std::string, std::vector<std::string> > >&
        , const std::set<std::string> &equalTypes) const;

    bool check_deref_vars(const Token* tokFunc, std::set<gt::SVarEntry>& derefVars);

    bool check_return_vars(const Token* tokFunc, int& memberVarId);

    bool check_assigned_vars(const Token* tokFunc, std::vector<Variable*>& assignedVars, int& rval);

    const std::set<gt::CFuncData::ParamRetRelation> * check_ret_param_relation(const gt::CFunction *gtFunc);
    const gt::CFunction* find_function_data(const Token* tokFunc);
    bool check_if_return(const Token *tok) const;
    bool is_var_init_in_func(const gt::CFunction *gf, const Variable *var, const std::string& mem) const;
    const static std::string find_full_function_name(const Token *tok);

    const gt::CScope* get_global_scope(const Variable* pVar);
    const gt::CField* get_field_info_by_token(const Token* tokVar);

    void set_analyze(bool bAnalyze) { m_bAnalyze = bAnalyze; }
    bool is_analyze() const { return m_bAnalyze; }

    const std::set<std::string>& get_risk_types();
    std::map<std::string, std::set<SPack1Scope> >& GetPack1Scope();

private:
    const gt::CFunction* FindFunctionWrapper(const Token* tokFunc) const;
    const gt::CFunction* FindFunction(const Token* tokFunc, bool requireScope = false) const;
    const gt::CFunction* FindFunction(const gt::CScope* gtScope, const Token* tokFunc, bool requireScope = false, bool requireConst = false) const;
    const gt::CScope* FindScope(const Scope* scope, const std::list<std::string>& missedScope) const;
    void FindFunctionInBase(const gt::CScope* gtScope, const Token* tok, size_t args, bool requireScope, std::vector<const gt::CFunction *> & matches, int level = 0) const;

    const gt::CScope* FindGtScopeByStringType(const Scope* currScope, const std::vector < std::string >& stringType) const;
private:
    CGlobalTokenizer();
    ~CGlobalTokenizer();

    std::map<void*, CGlobalTokenizeData*> m_threadData;
    CGlobalTokenizeData m_oneData;
    std::set<std::string> m_oneLuaData;
    bool m_bAnalyze;
};

class CPPCHECKLIB CGlobalErrorList
{
public:
    static CGlobalErrorList* Instance();
    static CGlobalErrorList* s_instance;

    std::list< std::string >& GetThreadErrorList(void* pKey);
    
    std::set<std::string>& GetOneData();

    void merge(Settings& setting);

private:

    CGlobalErrorList();
    ~CGlobalErrorList();

    std::map<void*, std::list<std::string> > m_threadData;
    std::set<std::string> m_oneData;
};

struct FuncRetInfo
{
    enum Operation
    {
        Unknown,
        Assign,
        Dereference,
        Use,
        CheckNull,
    };

    Operation Op;
    unsigned LineNo;
    std::string VarName;

    FuncRetInfo() : Op(Unknown), LineNo(0)
    {
    }

    FuncRetInfo(Operation op, unsigned line, std::string name)
        : Op(op), LineNo(line), VarName(name)
    {
    }

    static FuncRetInfo UnknownInfo;
};

struct FuncRetStatus
{
    struct ErrorMsg
    {
        std::string UsedFile;
        unsigned UsedLine;
        std::string UsedVarName;

        ErrorMsg() : UsedLine(0) { }
        ErrorMsg(const std::string& fileName, unsigned line, const std::string& varName)
            : UsedFile(fileName), UsedLine(line), UsedVarName(varName)
        {
        }
    };

    unsigned CheckNullCount;
    unsigned UsedCount;

    FuncRetStatus() : CheckNullCount(0), UsedCount(0)
    {
    }

    std::list<ErrorMsg> UsedList;
};

struct ArrayIndexInfo
{
    std::string BoundStr;
    std::string BoundType;
    unsigned BoundLine;
    std::string ArrayStr;
    std::string ArrayType;
    unsigned ArrayLine;
};

struct ArrayIndexRetInfo
{
    
};

struct StatisticThreadData
{
    std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > > FuncRetNullInfo;

    std::map<std::string, std::list<ArrayIndexInfo> > OutOfBoundsInfo;

    void Clear()
    {
        FuncRetNullInfo.clear();
    }

    void dump(const std::string& file_suffix) const;
    void dump_func_ret_null(const std::string& file_suffix) const;
    void dump_out_of_bounds(const std::string& file_suffix) const;
};

struct StatisticMergedData
{
    struct Pos
    {
        std::string FilePath;
        unsigned BoundaryLine;
        unsigned ArrayLine;
    };

    std::map<const gt::CFunction*, FuncRetStatus> FuncRetNullInfo;

    std::map<std::string, std::map<std::string, std::vector<Pos> > > OutOfBoundsInfo;
    
    void dump();

    void dump_func_ret_null();

    void dump_out_of_bounds();
};

class CPPCHECKLIB CGlobalStatisticData
{
public:
    static CGlobalStatisticData* Instance();
    static CGlobalStatisticData* s_instance;

    std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >& get_func_ret_null_thread_data(void* pKey);
    std::map<const gt::CFunction*, FuncRetStatus>& get_func_ret_null_merged_data();

    std::map<std::string, std::list<ArrayIndexInfo> >& get_out_of_bounds_thread_data(void* pKey);
    
    void merge(bool bDump);

    void merge_internal(StatisticThreadData& tempData, bool bDump);

    void handle_data(StatisticThreadData& tempData, bool bDump);

    void handle_func_ret_null(StatisticThreadData &tempData);
    void handle_out_of_bound(StatisticThreadData &tempData);

    void report_errors(Settings& setting, std::set<std::string>& errorList);

    void report_func_ret_null_errors(Settings& setting, std::set<std::string>& errorList);
    void report_out_of_bounds_errors(Settings& setting, std::set<std::string>& errorList);

private:
    void dump_merged_data();
    void dump_thread_data(const StatisticThreadData& data, const std::string& file_suffix);

private:

    CGlobalStatisticData();
    ~CGlobalStatisticData();

    std::map<void*, StatisticThreadData > m_threadData;
    StatisticMergedData m_mergedData;

    TSC_LOCK m_lock;
};

#ifdef USE_GLOG
class CPPCHECKLIB CLog
{
public:
    static void Initialize();

private:
    static bool s_initFlag;
};
#endif

#endif // BCACPP_GLOBALTOKENIZER_H
