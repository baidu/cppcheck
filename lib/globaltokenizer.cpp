//
//  globaltokenizer.cpp
//
//
//
#include <assert.h>

#include "globaltokenizer.h"
#include "tokenize.h"
#include "symboldatabase.h"
#include "settings.h"
#include "filedepend.h"
#include "path.h"
#include <fstream>
#ifdef USE_GLOE
#include "glog/logging.h"
#endif // USE_GLOE

#ifdef _WIN32
#pragma warning( disable : 4503)
#endif // _WIN32

typedef std::multimap < std::string, gt::CScope*>::const_iterator GlobalScopeCI;

CGlobalTokenizer* CGlobalTokenizer::s_instance;

void CGlobalTokenizer::uninitialize()
{
    SAFE_DELETE(s_instance);
}

CGlobalTokenizer* CGlobalTokenizer::Instance()
{
    if (!s_instance) {
        s_instance = new CGlobalTokenizer();
    }
    return s_instance;
}

void CGlobalTokenizer::merge(bool dump)
{
    ////bool b_record_func_info = Settings::Instance()->recordFuncinfo();
    ////if (b_record_func_info)
    {
        //clear file before content is written
        std::ofstream my_funcinfo("funcinfo.txt", std::ios::trunc);
    }
    for (std::map<void*, CGlobalTokenizeData*>::iterator i = m_threadData.begin(),
        e = m_threadData.end(); i != e; ++i)
    {
        if (dump)
        {
            static unsigned index = 0;
            CFileDependTable::create_log_directory();
            std::stringstream ss;
            ss << CFileDependTable::get_program_directory();
            ss << "log/gt_data_" << index++ << ".log";
            i->second->dump(ss.str().c_str());
        }

        m_oneData.merge(*(i->second));
        ////if (b_record_func_info)
        {
            i->second->dump_funcinfo();
        }
        ////tsc
        ////if (Settings::Instance()->checkLua()) {
            ////I->second->MergeLuaInfoToOneData(m_oneLuaData, m_oneData.GetExportClass());
        ////}
        SAFE_DELETE(i->second);
    }
    m_threadData.clear();

    m_oneData.RelateTypeInfo();

    const_cast<gt::CGlobalScope*>(m_oneData.GetData())->RecordFunc();
    //    dump info for lua
    /*if (Settings::Instance()->checkLua())
    {
        std::ofstream myLuainfo(Settings::Instance()->luaPath+"/cpp.lua.exp", std::ios::out);
        if (myLuainfo)
        {
            for (std::set<std::string>::const_iterator iter = m_oneLuaData.begin(), end = m_oneLuaData.end(); iter != end; ++iter)
            {
                myLuainfo << "[exp]" << *iter << "\n";
            }
            for (std::set<std::string>::const_iterator iter = m_oneData.GetExportClass().begin(), end = m_oneData.GetExportClass().end();
                iter != end; ++iter)
            {
                myLuainfo << "[lua.exp.cls]" << *iter << "\n";
            }
            m_oneData.GetExportClass().clear();
            m_oneLuaData.clear();
        }
    }*/
    if (dump)
    {
        std::stringstream ss;
        ss << CFileDependTable::get_program_directory();
        ss << "log/gt_data_merged.log";
        m_oneData.dump(ss.str().c_str());
    }
}

CGlobalTokenizeData* CGlobalTokenizer::get_global_data(void* pKey)
{
    if (!m_threadData.count(pKey))
    {
        m_threadData[pKey] = new CGlobalTokenizeData;
    }
    
    return m_threadData[pKey];
}

CGlobalTokenizer::CGlobalTokenizer() : m_bAnalyze(false)
{
    
}

CGlobalTokenizer::~CGlobalTokenizer()
{
    for (std::map<void*, CGlobalTokenizeData*>::iterator I = m_threadData.begin(), E = m_threadData.end(); I != E; ++I)
    {
        SAFE_DELETE(I->second);
    }
    
}

gt::CFuncData::RetNullFlag CGlobalTokenizer::check_func_return_null(const Token* tokFunc)
{
    if (!tokFunc)
    {
        return gt::CFuncData::RF_fUnknown;
    }
    const gt::CFunction* gtFunc = FindFunctionWrapper(tokFunc);
    if (!gtFunc)
    {
        return gt::CFuncData::RF_fUnknown;
    }

    return gtFunc->GetFuncData().GetFuncRetNull();
}

bool CGlobalTokenizer::check_func_exit(const Token *tok) const
{
    if (!tok)
    {
        return false;
    }
    const gt::CFunction* gtFunc = FindFunctionWrapper(tok);
    if (!gtFunc)
    {
        return false;
    }

    return gtFunc->GetFuncData().FunctionExit();
}

bool CGlobalTokenizer::check_deref_vars(const Token* tokFunc, std::set<gt::SVarEntry>& derefVars)
{
    if (!tokFunc)
    {
        return false;
    }

    const gt::CFunction* gtFunc = FindFunctionWrapper(tokFunc);
    if (!gtFunc)
    {
        return false;
    }

    const std::set<gt::SVarEntry>& derefSet = gtFunc->GetFuncData().GetDerefedVars();
    // todo, parameter to argument

    while (tokFunc->astParent() && tokFunc->astParent()->str() != "(")
    {
        tokFunc = tokFunc->astParent();
    }

    if (tokFunc->astParent())
    {
        std::vector<const Token*> args;
        for (std::set<gt::SVarEntry>::const_iterator I = derefSet.begin(), E = derefSet.end(); I != E; ++I)
        {
            const gt::SVarEntry& entry = *I;
            if (entry.eType == Argument)
            {
                if (args.empty())
                {
                    get_params_by_ast(tokFunc->astParent(), args);
                }

                // paramter to argument
                if (!args.empty() && (int)args.size() > entry.iParamIndex)
                {
                    const Token* tokArgTop = args[entry.iParamIndex];
                    SExprLocation exprLoc = create_sexpr_by_expr_tok(tokArgTop);
                    if (!exprLoc.Empty())
                    {
                        gt::SVarEntry newEntry = entry;
                        newEntry.sName = exprLoc.Expr.ExprStr;
                        derefVars.insert(newEntry);
                    }
                }
            }
            else
            {
                derefVars.insert(entry);
            }

        }
    }

    return true;
}

bool CGlobalTokenizer::check_return_vars(const Token* tokFunc, int& memberVarId)
{
    return false;
}

bool CGlobalTokenizer::check_assigned_vars(const Token* tokFunc, std::vector<Variable*>& assignedVars, int& rval)
{
    return false;
}

const gt::CFunction* CGlobalTokenizer::FindFunction(const Token* tok, bool requireScope) const
{
    
    if (!Token::Match(tok, "%name% ("))
    {
        return nullptr;
    }

    //LOG(INFO) << "Find Func: " << tok->str();

    //const std::multimap < std::string, gt::CScope*>& globalScope = m_oneData.GetData()->GetGlobalScopeMap();

    // find the scope this function is in
    const Scope *currScope = tok->scope();
    const Scope * scopeFunc = nullptr;
    while (currScope && currScope->isExecutable()) 
    {
        if (currScope->isFunction())
        {
            scopeFunc = currScope;
        }

        if (currScope->functionOf)
            currScope = currScope->functionOf;
        else
            currScope = currScope->nestedIn;
    }

    std::list<std::string> missedScope;
    if (scopeFunc)
    {
        if (const Token* tokDef = scopeFunc->classDef)
        {
            const Token* tok2 = tokDef->previous();
            while (tok2 && tok2->str() == "::" && tok2->previous() && tok2->previous()->isName())
            {
                const std::string& scopeStr = tok2->previous()->str();
                if (scopeStr != currScope->className)
                {
                    missedScope.push_front(scopeStr);
                }
                else
                    break;
                
                tok2 = tok2->tokAt(-2);
            }
        }
    }

    const gt::CScope* gtScope = FindScope(currScope, missedScope);
    if (!gtScope)
    {
        return nullptr;
    }    

    // check for a qualified name and use it when given
    if (tok->strAt(-1) == "::") 
    {
        // find start of qualified function name
        const Token *tok1 = tok;

        while (Token::Match(tok1->tokAt(-2), "%type% ::")) {
            tok1 = tok1->tokAt(-2);
        }

        // check for global scope
        if (tok1->strAt(-1) == "::") 
        {
            gtScope = m_oneData.GetData();
            gtScope = gtScope->FindChildScope(tok1->str());
        }

        // find start of qualification
        else 
        {
            while (gtScope) 
            {
                if (gtScope->GetName() == tok1->str())
                    break;
                else 
                {
                    const gt::CScope* gtScope2 = gtScope->FindChildScope(tok1->str());

                    if (gtScope2) 
                    {
                        gtScope = gtScope2;
                        break;
                    }
                    else
                        gtScope = gtScope->GetParent();
                }
            }
        }

        if (gtScope) {
            while (gtScope && !Token::Match(tok1, "%type% :: %any% (")) {
                tok1 = tok1->tokAt(2);
                gtScope = gtScope->FindChildScope(tok1->str());
            }

            tok1 = tok1->tokAt(2);

            if (gtScope && tok1)
            {
                //LOG(INFO) << "[Scope] " << gtScope->GetName() << "\t[Func] " << tok->str();
                return FindFunction(gtScope, tok1, requireScope);
            }
        }
    }
    // check for member function
    else if (Token::Match(tok->tokAt(-2), "!!this .")) 
    {
        const Token *tok1 = tok->tokAt(-2);
        if (Token::Match(tok1, "%var% .")) 
        {
            const Variable *var = currScope->check->getVariableFromVarId(tok1->varId());
            if (var)
            {
                if (const Scope* scope2 = var->typeScope())
                {
                    std::list<std::string> missedScope2;
                    if (const gt::CScope* gtScope2 = FindScope(scope2, missedScope2))
                    {
                        //LOG(INFO) << "[Scope] " << gtScope->GetName() << "\t[Func] " << tok->str();
                        return FindFunction(gtScope2, tok, requireScope);
                    }
                    else
                    {
                    }
                }
                else
                {
                    currScope = var->scope();
                    std::vector<std::string> stringType;
                    for (const Token* tokTmp = var->typeStartToken(); tokTmp && tokTmp != var->typeEndToken()->next(); tokTmp = tokTmp->next())
                    {
                        stringType.push_back(tokTmp->str());
                    }

                    gtScope = FindGtScopeByStringType(currScope, stringType);
                    if (gtScope) {
                        //LOG(INFO) << "[Scope] " << gtScope->GetName() << "\t[Func] " << tok->str();
                        return FindFunction(gtScope, tok, requireScope);
                    }
                }
            }
        }
        else if (Token::Match(tok1, ") ."))
        {
            if (const Token* tok2 = tok1->link()) 
            {
                if (tok2 && tok2->previous())
                {
                    if (const gt::CFunction* gtFunc = FindFunction(tok2->previous(), requireScope))
                    {
                        const std::vector<std::string>& funcReturn = gtFunc->GetReturn();
                        gtScope = FindGtScopeByStringType(currScope, funcReturn);
                        if (gtScope) {
                            //LOG(INFO) << "[Scope] " << gtScope->GetName() << "\t[Func] " << tok->str();
                            return FindFunction(gtScope, tok, requireScope);
                        }
                    }
                }
            }
        }
    }
    // check in enclosing scopes
    //else 
    {
        while (gtScope) 
        {
            //LOG(INFO) << "[Scope] " << gtScope->GetName() << "\t[Func] " << tok->str();
            const gt::CFunction* func = FindFunction(gtScope, tok, requireScope);
            if (func)
                return func;
            gtScope = gtScope->GetParent();
        }
    }

    return nullptr;
}

const gt::CFunction* CGlobalTokenizer::FindFunction(const gt::CScope* gtScope, const Token* tok, bool requireScope, bool requireConst) const
{
    if (!tok->scope())
        return nullptr;

    // make sure this is a function call
    const Token *end = tok->linkAt(1);
    if (!end)
        return nullptr;

    std::vector<const Token *> arguments;

    // find all the arguments for this function call
    const Token *arg = tok->tokAt(2);
    while (arg && arg != end) {
        arguments.push_back(arg);
        arg = arg->nextArgument();
    }

    std::vector<const gt::CFunction *> matches;

    const std::multimap<std::string, gt::CFunction*>& functionMap = gtScope->GetFunctionMap();

    // find all the possible functions that could match
    const std::size_t args = arguments.size();
    for (std::multimap<std::string, gt::CFunction*>::const_iterator it = functionMap.find(tok->str());
    it != functionMap.end() && it->first == tok->str(); ++it)
    {
        const gt::CFunction *func = it->second;

        if (args == func->GetArgCount() || (args < func->GetArgCount() && args >= func->GetMinArgCount()))
        {
            if (!requireScope || func->HasScope())
            {
                matches.push_back(func);
            }
        }
    }

    if (matches.empty())
    {
        // check in base classes
        FindFunctionInBase(gtScope, tok, args, requireScope, matches);
    }
    
    if (matches.empty()) 
    {
        return nullptr;
    }
    else
    {
        return matches[0];
    }
}

const gt::CScope* CGlobalTokenizer::FindScope(const Scope* scope, const std::list<std::string>& missedScope) const
{
    if (!scope || scope->isExecutable())
    {
        return nullptr;
    }

    std::list<std::string> typeList;
    while (scope)
    {
        typeList.push_front(scope->className);
        scope = scope->nestedIn;
    }
    if (!missedScope.empty())
        typeList.insert(typeList.end(), missedScope.begin(), missedScope.end());

    const gt::CScope* gtScope = m_oneData.GetData();
    std::list<std::string>::const_iterator I = typeList.begin();
    std::list<std::string>::const_iterator E = typeList.end();
    if (*I == emptyString)
        ++I;
    for (; gtScope && I != E; ++I)
    {
        gtScope = gtScope->FindChildScope(*I);
    }
    
    return gtScope;
}

void CGlobalTokenizer::FindFunctionInBase(const gt::CScope* gtScope, const Token* tok, size_t args, bool requireScope, std::vector<const gt::CFunction *> & matches, int level) const
{
    if (gtScope->GetKind() != gt::CScope::Type)
    {
        return;
    }
    if (const gt::CType* gtType = gtScope->GetAs<const gt::CType>()) 
    {
        const gt::CScope* parentScope = gtType->GetParent();
        if (parentScope)
        {
            const std::map<std::string, gt::CScope*>& scopeMap = parentScope->GetScopeMap();
            const std::set< std::string >& derived = gtType->GetDerived();

            for (std::set< std::string >::const_iterator I = derived.begin(), E = derived.end(); I != E; ++I)
            {
                const std::string& sDerived = *I;
                if (sDerived == gtScope->GetName())
                {
                    continue;
                }
                std::map<std::string, gt::CScope*>::const_iterator I2 = scopeMap.find(sDerived);
                if (I2 != scopeMap.end())
                {
                    gt::CScope* gtScope2 = I2->second;
                    const std::multimap<std::string, gt::CFunction*>& functionMap = gtScope2->GetFunctionMap();

                    for (std::multimap<std::string, gt::CFunction*>::const_iterator it = functionMap.find(tok->str());
                    it != functionMap.end() && it->first == tok->str(); ++it)
                    {
                        const gt::CFunction *func = it->second;
                        if (args == func->GetArgCount() || (args < func->GetArgCount() && args >= func->GetMinArgCount())) 
                        {
                            if (!requireScope || func->HasScope())
                            {
                                matches.push_back(func);
                            }
                        }
                    }
                    if (matches.empty())
                    {
                        level++;
                        if (level >=10)
                        {
                            break;
                        }
                        FindFunctionInBase(gtScope2, tok, args, requireScope, matches, level);
                    }
                    else
                        break;;
                }
            }
        }
    }
}

const gt::CFunction* CGlobalTokenizer::FindFunctionWrapper(const Token* tokFunc) const
{
    Token* tok = const_cast<Token*>(tokFunc);
    const gt::CFunction* gtFunc = tok->GetTokenEx().GetGtFunc();
    if (gtFunc)
    {
        return gtFunc;
    }
    
    gtFunc = FindFunction(tokFunc, true);
    if (gtFunc)
    {
        tok->GetTokenEx().SetGtFunc(gtFunc);
    }
    return gtFunc;
}

const gt::CScope* CGlobalTokenizer::FindGtScopeByStringType(const Scope* currScope, const std::vector < std::string >& stringType) const
{
    if (stringType.size() == 0)
    {
        return nullptr;
    }
    
    // find the scope this function is in
    while (currScope && currScope->isExecutable()) {
        if (currScope->functionOf)
            currScope = currScope->functionOf;
        else
            currScope = currScope->nestedIn;
    }

    std::list<std::string> missedScope;
    const gt::CScope* gtScope = FindScope(currScope, missedScope);
    if (!gtScope)
    {
        return nullptr;
    }

    std::size_t flag = 0;

    while (flag < stringType.size() && (stringType[flag] == "const" || stringType[flag] == "struct")) {
        ++flag;
    }

    if (flag >= stringType.size())
    {
        return nullptr;
    }

    if (stringType[flag] == "::")
    {
        gtScope = m_oneData.GetData();
        flag = 1;
    }
    else
    {
        while (gtScope) {
            if (gtScope->GetName() == stringType[flag])
                break;
            else 
            {
                const gt::CScope* gtScope2 = nullptr;
                gtScope2 = gtScope->FindChildScope(stringType[flag]);

                if (!gtScope2)
                {
                    // try find using 
                    if (!gtScope->GetUsingInfo().empty())
                    {
                        for (std::set<std::string>::iterator I = gtScope->GetUsingInfo().begin(), E = gtScope->GetUsingInfo().end(); I != E; ++I)
                        {
                            const std::string& info = *I;

                            const gt::CScope* gtScope3 = gtScope->FindChildScope(info);

                            if (gtScope3)
                            {
                                gtScope2 = gtScope3->FindChildScope(stringType[flag]);
                                if (gtScope2)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }

                if (gtScope2) {
                    gtScope = gtScope2;
                    break;
                }
                else
                    gtScope = gtScope->GetParent();
            }
        }
    }
    
    if (gtScope) {
        while (gtScope && (flag + 2 < stringType.size()) && (stringType[flag + 1] == "::"))
        {
            flag += 2;
            gtScope = gtScope->FindChildScope(stringType[flag]);
        }
    }
    return gtScope;
}

CGlobalTokenizeData::CGlobalTokenizeData()
: m_data(new gt::CGlobalScope()),
  m_bRecoredExportClass(false)
{
    
}

CGlobalTokenizeData::~CGlobalTokenizeData()
{
    SAFE_DELETE(m_data);
}

void CGlobalTokenizeData::merge(const CGlobalTokenizeData& threadData)
{
    if (!m_data) {
        m_data = new gt::CGlobalScope();
    }
    m_data->merge(threadData.m_data);

    for (Pack1MapCI I = threadData.m_pack1Scopes.begin(), E = threadData.m_pack1Scopes.end(); I != E; ++I)
    {
        std::set<SPack1Scope>& set = m_pack1Scopes[I->first];
        for (std::set<SPack1Scope>::const_iterator I2 = I->second.begin(), E2 = I->second.end(); I2 != E2; ++I2)
        {
            set.insert(*I2);
        }
    }

    if (!threadData.m_riskTypes.empty())
    {
        m_riskTypes.insert(threadData.m_riskTypes.begin(), threadData.m_riskTypes.end());
    }
}

std::set<std::string> CGlobalTokenizeData::s_stdTypes = make_container<std::set<std::string> >()
<< "bool"
<< "char"
<< "char16_t"
<< "char32_t"
<< "double"
<< "float"
<< "int"
<< "long"
<< "short"
<< "size_t"
<< "void"
<< "wchar_t"
<< "int8_t"
<< "int16_t"
<< "int32_t"
<< "int64_t"
<< "uint8_t"
<< "uint16_t"
<< "uint32_t"
<< "uint64_t"
<< "std"
<< "boost";

void CGlobalTokenizeData::RelateTypeInfo()
{
    //std::set<gt::CScope> record;
    record_scope(m_data);
    for (SMI I = m_scopeMap.begin(), E = m_scopeMap.end(); I != E; ++I)
    {
        gt::CScope* scope = I->second;
        std::map<std::string, gt::CField*>& fm = scope->GetFieldMap();
        if (fm.empty())
        {
            continue;
        }

        for (std::map<std::string, gt::CField*>::iterator I2 = fm.begin(), E2 = fm.end(); I2 != E2; ++I2)
        {
            gt::CField* f = I2->second;

            const std::vector<std::string>& tl = f->GetType();
            if (tl.empty())
            {
                continue;
            }

            if (s_stdTypes.count(tl[0]))
            {
                continue;
            }

            gt::CScope* s = FindType(tl, scope);

            if (s)
            {
                f->SetGType(s);
            }

        }
    }
}

void CGlobalTokenizeData::InitScopes(const SymbolDatabase* symbolDB)
 {
    
    if (!m_data) {
        m_data = new gt::CGlobalScope();
    }
    
    if (symbolDB->scopeList.empty()) {
        return;
    }
    
    const Scope& firstScope = symbolDB->scopeList.front();
    if (!firstScope.isGlobal()) {
        return;
    }
    
    m_scopeCache.clear();
    std::set<const Scope*> added;
    sync_scopes(&firstScope, m_data, added);
}

void CGlobalTokenizeData::sync_scopes(const Scope* scope, gt::CScope* gtScope, std::set<const Scope*>& added)
{
    // sync need init
    if (Type* ptype = scope->definedType)
    {
        if (gtScope->GetNeedInit() < ptype->needInitialization)
        {
            gtScope->SetNeedInit(ptype->needInitialization);
        }
    }

    // sync using info
    const std::list<Scope::UsingInfo>& usinginfo = scope->usingList;
    for (std::list<Scope::UsingInfo>::const_iterator I = usinginfo.begin(), E = usinginfo.end(); I != E; ++I)
    {
        const Scope::UsingInfo& info = *I;
        if (info.start && Token::Match(info.start, "using namespace %type% ;"))
        {
            gtScope->AddUsingInfo(info.start->tokAt(2)->str());
        }
    }
    
    const std::list<Function>& funcList = scope->functionList;
    for (std::list<Function>::const_iterator I = funcList.begin(), E = funcList.end(); I != E; ++I)
    {
        gt::CFunction* gtFunc = new gt::CFunction(*I);
        gt::CScope* gtParent = gtScope;
        gt::CFunction* gtFindFunc = gtParent->TryGetFunc(gtFunc);
        if (!gtFindFunc)
        {
            gtFunc->InitFuncData(*I);
            gtParent->add_child_func(gtFunc);
        }
        else
        {
            gtFunc->InitFuncData(*I);
            gtFindFunc->MergeFunc(gtFunc);
            SAFE_DELETE(gtFunc);
        }
        added.insert(I->functionScope);
    }
    // add fields
    ////const Settings *setting = Settings::Instance();
    const std::list<Variable>& varList = scope->varlist;
    for (std::list<Variable>::const_iterator I = varList.begin(), E = varList.end(); I != E; ++I)
    {
        const Variable& var = *I;
        const std::string& varName = var.name();
        if (varName.empty())
        {
            continue;
        }
        Type::NeedInitialization need = Type::False;
        if (var.type())
        {
            need = var.type()->needInitialization;
        }
        else if (var.nameToken() 
            && var.nameToken()->previous() 
            && (Token::Match(var.nameToken()->previous(), "bool|char|short|int|long|float|double|wchar_t|unsigned|signed")
            ////    || nullptr != setting->library.podtype(var.nameToken()->previous()->str()))
            ))
        {
            need = Type::True;
        }
        gt::CField* newField = new gt::CField(var.name(), var.getBaseFlag(), var.getAccess(), need);

        const Token* tok = var.typeStartToken();
        const Token* tokE = var.typeEndToken()->next();
        std::vector<std::string>& filedType = newField->GetType();
        while (tok && tok != tokE)
        {
            filedType.push_back(tok->str());
            tok = tok->next();
        }
        if (var.isArray())
        {
            std::vector<gt::CField::Dimension>& fdimens = newField->GetDimensions();
            const std::vector<Dimension>& dimens = var.dimensions();
            for (std::vector<Dimension>::const_iterator I2 = dimens.begin(), E2 = dimens.end(); I2 != E2; ++I2)
            {
                const Dimension& d = *I2;
                if (d.known && d.num > 0)
                {
                    fdimens.push_back(gt::CField::Dimension((unsigned)d.num));
                }
                else
                {
                    if (d.start && d.start == d.end)
                    {
                        fdimens.push_back(gt::CField::Dimension(d.start->str()));
                    }
                    else
                    {
                        fdimens.push_back(gt::CField::Dimension());
                    }
                }

            }
        }

        gt::CField* f = gtScope->TryGetField(varName);

        if (!f)
        {
            gtScope->AddChildFiled(newField);
        }
        else
        {
            f->merge(newField);
        }
    }

    typedef std::list<Scope*>::const_iterator ScopeCI;
    const std::list<Scope*>& scopeList = scope->nestedList;
    for (ScopeCI I = scopeList.begin(), E = scopeList.end(); I != E; ++I) 
    {
        Scope* childScope = *I;

        if (childScope->isClassOrStruct() || childScope->isNamespace())
        {
            gt::CScope* gtChildScope = NULL;
            if (!childScope->className.empty())
            {

                gtChildScope = gtScope->TryGetChildScope(childScope->className);
                if (!gtChildScope)
                {
                    if (childScope->isClassOrStruct()) {
                        gtChildScope = new gt::CType(childScope);
                        m_scopeCache[childScope] = gtChildScope;
                    }
                    else if (childScope->isNamespace())
                    {
                        gtChildScope = new gt::CType(childScope->className);
                    }
                    else
                    {
                        //assert(false);
                    }

                    if (gtChildScope)
                        gtScope->add_child_scope(gtChildScope);
                }
                else
                {
                    if (gtChildScope->GetKind() == gt::CScope::Type)
                    {
                        gt::CType* pType = dynamic_cast<gt::CType*>(gtChildScope);
                        pType->MergeDerived(childScope);
                    }
                }
            }
            else
            {
                // namesapce { ... }
                gtChildScope = gtScope;
            }

            if (!gtChildScope)
                return;

            sync_scopes(childScope, gtChildScope, added);
        }
        else if (childScope->isFunction())
        {
            if (added.count(childScope))
            {
                continue;
            }

            const Token* tokFunc = childScope->classDef;

            const Token* tokEnd = tokFunc;
            const Token* tokClass = tokFunc;
            if (tokClass->previous() && tokClass->previous()->str() == "~")
            {
                tokEnd = tokClass = tokClass->previous();
            
            }

            while (tokClass && tokClass->tokAt(-2) && Token::Match(tokClass->tokAt(-2), "%type% ::"))
            {
                tokClass = tokClass->tokAt(-2);
            }

            gt::CScope* gtChildScope = gtScope;
            while (tokClass && tokClass != tokEnd)
            {
                gt::CScope* newGtScope = gtChildScope->TryGetChildScope(tokClass->str());
                if (!newGtScope)
                {
                    newGtScope = new gt::CType(tokClass->str());
                    gtChildScope->add_child_scope(newGtScope);
                }
                gtChildScope = newGtScope;
                tokClass = tokClass->tokAt(2);
            }

            gt::CFunction* gtFunc = new gt::CFunction(childScope);
            gt::CScope* gtParent = gtChildScope;
            gt::CFunction* gtFindFunc = gtParent->TryGetFunc(gtFunc);
            if (!gtFindFunc)
            {
                gtFunc->InitFuncData(childScope);
                gtParent->add_child_func(gtFunc);
            }
            else
            {
                gtFunc->InitFuncData(childScope);
                gtFindFunc->MergeFunc(gtFunc);
                
                SAFE_DELETE(gtFunc);
            }
        }
    }
}

void CGlobalTokenizeData::record_scope(gt::CScope* scope)
{
    const std::string& sName = scope->GetName();
    m_scopeMap.insert(std::pair<std::string, gt::CScope*>(sName, scope));

    const std::map<std::string, gt::CScope*>& scopeMap = scope->GetScopeMap();
    for (std::map<std::string, gt::CScope*>::const_iterator I = scopeMap.begin(), E = scopeMap.end(); I != E; ++I)
    {
        gt::CScope* s = I->second;
        record_scope(s);
    }
}

bool CheckScopeOK(const gt::CScope* scope, const std::vector<std::string>& kt)
{
    std::vector <std::string>::const_reverse_iterator RI = kt.rbegin();
    std::vector <std::string>::const_reverse_iterator RE = kt.rend();
    for (;scope && RI != RE; ++RI, scope = scope->GetParent())
    {
        if (*RI != scope->GetName())
        {
            return false;
        }
    }

    return RI == RE;
}

gt::CScope* TryGetScopeFromCurrent(gt::CScope* cs, const std::string& st)
{
    const std::set<std::string> ui = cs->GetUsingInfo();
    gt::CScope* s = cs->TryGetChildScope(st);
    if (!s)
    {
        for (std::set<std::string>::const_iterator I = ui.begin(), E = ui.end(); I != E; ++I)
        {
            gt::CScope* us = cs->TryGetChildScope(*I);
            if (us)
            {
                s = us->TryGetChildScope(st);
                if (s)
                {
                    break;
                }
            }
        }
    }
    return s;
}

gt::CScope* TryGetScopeFromCurrent(gt::CScope* cs, const std::vector<std::string>& kt)
{
    const std::set<std::string> ui = cs->GetUsingInfo();
    const std::string& st = *kt.begin();
    gt::CScope* s = TryGetScopeFromCurrent(cs, st);

    while (!s)
    {
        cs = cs->GetParent();
        if (cs)
        {
            s = TryGetScopeFromCurrent(cs, st);
        }
        else
            break;
    }

    if (s)
    {
        std::vector<std::string>::const_iterator I = kt.begin();
        ++I;
        for (std::vector<std::string>::const_iterator E = kt.end(); I != E; ++I)
        {
            s = s->TryGetChildScope(*I);
            if (!s)
            {
                break;
            }
        }
    }
    return s;
}

gt::CScope* CGlobalTokenizeData::FindType(const std::vector<std::string>& tl, gt::CScope* cs)
{
    std::size_t index = 0;
    if (tl[index] == "::")
    {
        cs = m_data;
        ++index;
    }

    std::vector<std::string> kt;

    while (index < tl.size())
    {
        const std::string& s = tl[index];
        if (isalpha(s[0]) || s[0] == '_')
        {
            kt.push_back(s);
        }
        else if (s == "<")
        {
            break;
        }
        index++;
    }

    gt::CScope* s = TryGetScopeFromCurrent(cs, kt);

    
    if (s)
    {
        return s;
    }
    else
    {
        const std::string& st = *kt.rbegin();
        for (SMI I = m_scopeMap.find(st), E = m_scopeMap.end(); I != E && I->first == st; ++I)
        {
            s = I->second;
            if (CheckScopeOK(s, kt))
            {
                return s;
            }
        }
    }
    return nullptr;
}

void CGlobalTokenizeData::dump(const char* szPath) const
{
    std::ofstream fs(szPath, std::ios_base::trunc);
    if (fs.bad())
        return;

    fs << "[Global Types]" << std::endl;
    m_data->dump(fs);

    fs.close();
}

const gt::CFunction* CGlobalTokenizer::find_function_data(const Token* tokFunc)
{
    if (!tokFunc)
    {
        return nullptr;
    }
    return FindFunctionWrapper(tokFunc);
}

const std::set<gt::CFuncData::ParamRetRelation> * CGlobalTokenizer::check_ret_param_relation(const gt::CFunction *gtFunc)
{
    if (!gtFunc)
    {
        return nullptr;
    }
    const std::set<gt::CFuncData::ParamRetRelation> *rl = gtFunc->GetFuncData().GetRetParamRelation();
    if (rl[0].empty() && rl[1].empty())
    {
        return nullptr;
    }
    return rl;
}

bool CGlobalTokenizer::check_if_match_signature(const Token *tokFunc,
    const std::multimap<std::string, std::pair<std::string, std::vector<std::string> > >& funcNotRetNull,
    const std::set<std::string> &equalTypes) const
{
    // todo: consider foo<T, T>(1, 2)
    if (!tokFunc || !tokFunc->next() || !tokFunc->tokAt(2))
    {
        return false;
    }

    typedef std::multimap<std::string, std::pair<std::string, std::vector<std::string> > > SIG_MAP;
    //typedef std::pair<std::string, std::vector<std::string> > SIG;
    std::pair<SIG_MAP::const_iterator, SIG_MAP::const_iterator > ret;
    ret = funcNotRetNull.equal_range(tokFunc->str());

    if (ret.first == ret.second)
    {
        return false;
    }
    //spacial case if only function name is given
    ////Settings *pSettings = Settings::Instance();
    for (SIG_MAP::const_iterator iter = ret.first; iter != ret.second; ++iter)
    {
        ////if (iter->second.second.size() == 1 && iter->second.second[0] == pSettings->ANY_TYPE_T)
        {
            return true;
        }
    }

    // find function parameter type
    std::vector<std::string> varTypes;
    for (const Token *tok = tokFunc->tokAt(2); tok; tok = tok->nextArgument())
    {
        while (tok && tok->next() && tok->next()->str() == ".")
        {
            tok = tok->tokAt(2);
        }
        if (!tok)
        {
            return false;
        }
        //todo: pass 0 to pointer parameter
        if (tok->tokType() == Token::eChar || tok->tokType() == Token::eNumber)
        {
            varTypes.push_back("int");
        }
        else if (tok->variable())
        {
            const Variable* pVar = tok->variable();
            std::string typeStr;
            for (const Token *varType = pVar->typeStartToken(); varType && varType != pVar->typeEndToken(); varType = varType->next())
            {
                typeStr += varType->str();
                typeStr += " ";
            }
            if (pVar->typeEndToken())
            {
                typeStr += pVar->typeEndToken()->str();
                typeStr += " ";
            }

            typeStr = typeStr.substr(0, typeStr.size() - 1);
            if (equalTypes.find(typeStr) != equalTypes.end())
            {
                varTypes.push_back("int");
            }
            else
            {
                varTypes.push_back(typeStr);
            }
        }
        else
        {
            ////varTypes.push_back(pSettings->ANY_TYPE_T);
        }
    }

    //get className
    std::string className;
    if (tokFunc->function())
    {
        const Scope *pScope = tokFunc->function()->nestedIn;
        while (pScope)
        {
            if (pScope->className.empty())
            {
                pScope = pScope->nestedIn;
                continue;
            }
            className = pScope->className + "::" + className;
            pScope = pScope->nestedIn;
        }
        if (!className.empty())
        {
            className = className.substr(0, className.size() - 2);
        }
    }
    
    //save equal types end for efficiency
    for (SIG_MAP::const_iterator iter = ret.first; iter != ret.second; ++iter)
    {
        //todo: this comparison is not so strict
        if (className == iter->second.first ||
            className.find_last_of(iter->second.first) != className.npos 
            || iter->second.first.find_last_of(className) != iter->second.first.npos )
        {
            if (varTypes.size() != iter->second.second.size())
            {
                continue;
            }
            bool bEqual = true;
            
            for (std::vector<std::string>::const_iterator iterLib = varTypes.begin(), libEnd = varTypes.end(),
                iterSig = varTypes.begin(); iterLib != libEnd; ++iterLib)
            {
                ////if (*iterLib != *iterSig && *iterSig != pSettings->ANY_TYPE_T)
                {
                    bEqual = false;
                    break;
                }
            }
            if (bEqual)
            {
                return true;
            }
        }
    }
    
    return false;
}

bool CGlobalTokenizer::check_if_return(const Token *tok) const
{
    if (!tok->isName())
    {
        return false;
    }
    ////const Settings *pSettings = Settings::Instance();
    ////if (Token::Match(tok, pSettings->returnKeyWord)
    ////    || pSettings->library.functionexit.find(FindFullFunctionName(tok)) != pSettings->library.functionexit.end())
    {
        return true;
    }
    if (tok->next() 
        &&(tok->next()->str()== "(" 
            || (tok->next()->str() == "<" 
                && tok->linkAt(1)
                && tok->linkAt(1)->next()
                && tok->linkAt(1)->next()->str() == "("))
                && check_func_exit(tok)
        )
    {
        return true;
    }

    return false;
}

bool CGlobalTokenizer::is_var_init_in_func(const gt::CFunction *gf, const Variable *var, const std::string &mem) const
{
    const std::set<gt::SOutScopeVarState> &os = gf->GetFuncData().GetOutScopeVarState();
    if (os.empty())
    {
        return false;
    }
    std::string str = mem;
    if (var->typeEndToken() && var->typeEndToken()->isName())
    {
        str = var->typeEndToken()->str() + ":" + mem;
    }

    for (const Token *t = var->typeStartToken(), *e = var->typeEndToken(); t != e; t = t->next())
    {
        if (!t->isKeyword() && t->tokType() == Token::eName)
        {
            str = t->str() + ":" + mem;
            break;
        }
    }
    
    str = ":" + str;
    for (std::set<gt::SOutScopeVarState>::const_iterator iter = os.begin(), end = os.end(); iter != end; ++iter)
    {
        if (iter->strName == str)
        {
            return true;
        }
    }
    return false;
}

const std::string CGlobalTokenizer::find_full_function_name(const Token *tok)
{
    if (!tok)
    {
        return std::string();
    }
    if (!tok->previous())
    {
        return tok->str();
    }
    const Token *tokPrev = tok->previous();
    if (tokPrev->str() == ".")
    {
        if (tokPrev->previous() && tokPrev->previous()->variable())
        {
            std::string strName = "";
            const Variable *pVar = tokPrev->previous()->variable();
            for (const Token *tokLoop = pVar->typeStartToken(); tokLoop && tokLoop != pVar->typeEndToken(); tokLoop = tokLoop->next())
            {
                strName += tokLoop->str();
            }
            if (pVar->typeEndToken())
            {
                strName += pVar->typeEndToken()->str();
            }

            if (strName.empty())
            {
                return tok->str();
            }
            else
            {
                strName += "::";
                strName += tok->str();
            }
            if (0 == strName.compare(0, 2, "::"))
            {
                strName = strName.substr(2);
            }
            return strName;
        }
        //maybe string::find
        else
        {
            return std::string();
        }
    }
    else if (tokPrev->str() == "::")
    {
        const Token *tokLoop = tokPrev;
        std::string strName = "";
        while (tokLoop)
        {
            if (tokLoop->str() == "::")
            {
                strName = "::" + strName;
            }
            else if (tokLoop->str() == ">")
            {
                if (tokLoop->link())
                {
                    std::string strLink = "";
                    for (const Token *t = tokLoop->link(); t && t != tokLoop; t = t->next())
                    {
                        strLink += t->str();
                    }
                    strName = strLink + ">" + strName;
                }
                else
                {
                    return std::string();
                }
                tokLoop = tokLoop->link();
            }
            else if (tokLoop->str() == "std")
            {
                strName = "std" + strName;
                break;
            }
            else if (tokLoop->isName() && !Token::Match(tokLoop, "return | throw"))
            {
                strName = tokLoop->str() + strName;
            }
            else
            {
                break;
            }
            tokLoop = tokLoop->previous();
        }
        strName += tok->str();
        if (0 == strName.compare(0, 2, "::"))
        {
            strName = strName.substr(2);
        }
        return strName;
    }

    return tok->str();
}

const gt::CScope* CGlobalTokenizer::get_global_scope(const Variable* pVar)
{
    if (!pVar)
        return nullptr;

    const Scope* currScope = pVar->nameToken()->get_current_scope();

    if (!currScope)
        return nullptr;

    std::vector<std::string> typeVec;
    const Token* tok = pVar->typeStartToken();
    const Token* tok2 = pVar->typeEndToken()->next();

    if (tok && tok2)
    {
        for (; tok != tok2; tok = tok->next())
        {
            typeVec.push_back(tok->str());
        }
    }

    const gt::CScope* gScope = FindGtScopeByStringType(currScope, typeVec);
    return gScope;
}

const gt::CField* CGlobalTokenizer::get_field_info_by_token(const Token* tokVar)
{
    if (!tokVar)
    {
        return nullptr;
    }

    const Scope* scope = tokVar->get_current_scope();
    if (!scope)
    {
        return nullptr;
    }
    const Token* tok = tokVar;
    const Variable* pVar = tok->variable();
    while (!pVar && Token::Match(tok->tokAt(-2), "%name% ."))
    {
        tok = tok->tokAt(-2);
        pVar = tok->variable();
    }

    if (pVar)
    {
        if (tok == tokVar)
        {
            const Scope* s = pVar->nameToken()->get_current_scope();
            if (s)
            {
                std::list<std::string> missed;
                const gt::CScope* gts = FindScope(s, missed);
                if (!gts)
                {
                    return nullptr;
                }
                const gt::CField* gtF =  gts->TryGetField(pVar->name()) ;

                return gtF;
            }
            else
                return nullptr;
        }
        else
        {
            const gt::CScope* gts = get_global_scope(pVar);
            
            if (gts)
            {
                tok = tok->tokAt(2);
                const gt::CField* gtF = gts->TryGetField(tok->str());
                while (gtF && tok != tokVar)
                {
                    gts = gtF->GetGType();
                    tok = tok->tokAt(2);
                    if (gts)
                        gtF = gts->TryGetField(tok->str());
                    else
                        gtF = nullptr;
                }

                return gtF;
            }
        }
    }
    else
    {
        return nullptr;
    }

    return nullptr;
}

const std::set<std::string>& CGlobalTokenizer::get_risk_types()
{
    return m_oneData.get_risk_types();
}

std::map<std::string, std::set<SPack1Scope> >& CGlobalTokenizer::GetPack1Scope()
{
    return m_oneData.GetPack1Scope();
}

CGlobalErrorList* CGlobalErrorList::s_instance;

std::list< std::string >& CGlobalErrorList::GetThreadErrorList(void* pKey)
{
    return m_threadData[pKey];
}

std::set<std::string>& CGlobalErrorList::GetOneData()
{
    return m_oneData;
}

void CGlobalErrorList::merge(Settings& setting)
{
    // filter same filter with out funcinfo
    std::map<std::string, std::string> msgWithoutFuncInfoSet;
    for (std::map<void*, std::list<std::string> >::iterator I = m_threadData.begin(), E = m_threadData.end(); I != E; ++I)
    {
        for (std::list<std::string>::iterator I2 = I->second.begin(), E2 = I->second.end(); I2 != E2; ++I2)
        {
            const std::string& sMsg = *I2;
            std::string::size_type pos = sMsg.find("func_info=");
            
            if (pos != std::string::npos)
            {
                std::string sMsg2 = sMsg.substr(0, pos);
                std::string sMsg3 = sMsg.substr(pos);

                if (!msgWithoutFuncInfoSet.count(sMsg2))
                {
                    msgWithoutFuncInfoSet[sMsg2] = sMsg3;
                }
                else
                {
                    const std::string& oldFuncInfo = msgWithoutFuncInfoSet[sMsg2];
                    if (sMsg3 > oldFuncInfo)
                    {
                        msgWithoutFuncInfoSet[sMsg2] = sMsg3;
                    }
                }
            }
            else
            {
                m_oneData.insert(sMsg);
            }
        }
    }

    if (!msgWithoutFuncInfoSet.empty())
    {
        for (std::map<std::string, std::string>::iterator I = msgWithoutFuncInfoSet.begin(), E = msgWithoutFuncInfoSet.end(); I != E; ++I)
        {
            m_oneData.insert(I->first + I->second);
        }
    }

    ////CGlobalStatisticData::Instance()->Merge(setting.debugDumpGlobal);
    CGlobalStatisticData::Instance()->report_errors(setting, m_oneData);
}

CGlobalErrorList* CGlobalErrorList::Instance()
{
    if (nullptr == s_instance)
    {
        s_instance = new CGlobalErrorList();
    }
    return s_instance;
}

CGlobalErrorList::CGlobalErrorList()
{

}

CGlobalErrorList::~CGlobalErrorList()
{

}

void CGlobalTokenizeData::record_func_info(const Tokenizer* tokenizer, const std::string &strFileName)
{
    const SymbolDatabase * const symbol_database = tokenizer->getSymbolDatabase();

    const std::size_t functions = symbol_database->functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i) {
        const Scope * scope = symbol_database->functionScopes[i];
        if (!scope)
            return;
        Token* tok = (Token*)scope->classStart;
        int startline = 0;
        int endline = 0;
        if (tok)
        {
            startline = tok->linenr();
            if (tok->link())
            {
                endline = tok->link()->linenr();
            }

            //fileindex==0
            if (startline>0 && endline>0)
            {
                FUNCINFO temp;
                temp.filename = tokenizer->list.file(tok); 
                ErrorLogger::GetScopeFuncInfo(scope, temp.functionname);
                temp.startline = startline;
                temp.endline = endline;
                m_funcInfoList.push_back(temp);
            }
        }
    }
}

/**
* 3 <= name length <= 32
*/
void CGlobalTokenizeData::record_info_for_lua(const Tokenizer* tokenizer, const std::string &strFileName)
{
    /*if (!Settings::Instance()->checkLua()) {
        return;
    }*/

    std::size_t n_length = 0;
    std::size_t n_index = 0;
    bool b_valid_name = false;
    for (const Token *tok = tokenizer->list.front(); tok; tok = tok->next())
    {
        if (!tok->isString())
        {
            continue;
        }
        const std::string &str = tok->str();
        n_length = str.length() - 1;
        if (n_length < 4 || n_length > 33)
        {
            continue;
        }
        if (str[1] != '_' && !::isalpha(str[1]))
        {
            continue;
        }
        b_valid_name = true;
        for (n_index = 2; n_index < n_length; ++n_index)
        {
            if (str[n_index] != '_' && !::isalnum(str[n_index]))
            {
                b_valid_name = false;
                break;
            }
        }
        if (b_valid_name)
        {
            m_InfoForLua.insert(str.substr(1, n_length - 1));
        }
    }
}

void CGlobalTokenizeData::dump_funcinfo()
{
    //append data to
    std::ofstream myFuncinfo("funcinfo.txt", std::ios::app);
    if (!myFuncinfo)
    {
        return;
    }
    for (std::list<FUNCINFO>::const_iterator iter = m_funcInfoList.begin(), end = m_funcInfoList.end(); iter != end; ++iter)
    {
        myFuncinfo << (*iter).filename << "#bebe#" << (*iter).functionname << "#bebe#" << (*iter).startline << "#bebe#" << (*iter).endline << "\n";
    }
}

namespace {
    const std::set<std::string> lua_keywords = make_container< std::set<std::string> >()
        << "and" << "break" << "do" << "else" << "elseif" << "end" << "false" << "for" << "function" << "goto" << "if"
        << "in" << "local" << "nil" << "not" << "or" << "repeat" << "return" << "then" << "true" << "until" << "while";
}
void CGlobalTokenizeData::merge_lua_info_to_one_data(std::set<std::string> & m_oneLuaData, std::set<std::string> &luaExportClass)
{
    for (std::set<std::string>::const_iterator iter = m_InfoForLua.begin(), end = m_InfoForLua.end();
        iter != end; ++iter)
    {
        if (iter->compare(0, 2, "m_") != 0 && lua_keywords.count(*iter) == 0)
        {
            m_oneLuaData.insert(*iter);
        }
    }
    luaExportClass.insert(m_luaExportClass.begin(), m_luaExportClass.end());
    m_InfoForLua.clear();
    m_luaExportClass.clear();
}

void CGlobalTokenizeData::add_pack_1_scope(const std::string& filename, unsigned start, unsigned end)
{
    std::set<SPack1Scope>& pack1_set = m_pack1Scopes[filename];
    pack1_set.insert(SPack1Scope(start, end));
}

std::map<std::string, std::set<SPack1Scope> >& CGlobalTokenizeData::GetPack1Scope()
{
    return m_pack1Scopes;
}

void CGlobalTokenizeData::record_risk_types(const Tokenizer* tokenizer)
{
    const SymbolDatabase * const symbol_database = tokenizer->getSymbolDatabase();
    std::vector<const Scope *>::const_iterator i = symbol_database->classAndStructScopes.begin();
    std::vector<const Scope *>::const_iterator e = symbol_database->classAndStructScopes.end();
    for (; i != e; ++i)
    {
        const Scope* typeScope = *i;
        const std::string& sFilePath = tokenizer->list.file(typeScope->classDef);
        const unsigned lineno = typeScope->classDef->linenr();
        
        if (m_pack1Scopes.count(sFilePath))
        {
            const std::set<SPack1Scope>& pack1Set = m_pack1Scopes[sFilePath];

            for (std::set<SPack1Scope>::const_iterator i2 = pack1Set.begin(), e2 = pack1Set.end(); i2 != e2; ++i2)
            {
                if (lineno > i2->StartLine && lineno < i2->EndLine)
                {
                    std::list<Variable>::const_iterator i3 = typeScope->varlist.begin();
                    std::list<Variable>::const_iterator e3 = typeScope->varlist.end();

                    for (; i3 != e3; ++i3)
                    {
                        const Variable& var = *i3;
                        if (var.isFloatingType() || (var.isIntegralType() && var.typeStartToken()->isLong()))
                        {
                            m_riskTypes.insert(typeScope->className);
                        }
                    }
                }
            }
        }
    }

    i = symbol_database->classAndStructScopes.begin();
    for (; i != e; ++i)
    {
        const Scope* typeScope = *i;
        std::list<Variable>::const_iterator i3 = typeScope->varlist.begin();
        std::list<Variable>::const_iterator e3 = typeScope->varlist.end();

        for (; i3 != e3; ++i3)
        {
            const Variable& var = *i3;
            if (is_risk_type(var, typeScope))
            {
                m_riskTypes.insert(typeScope->className);
            }
        }
    }
}

bool CGlobalTokenizeData::is_risk_type(const Variable& var, const Scope* scope)
{
    for (const Token* tok = var.typeStartToken(); tok != var.typeEndToken()->next(); tok = tok->next())
    {
        if (m_riskTypes.count(tok->str()))
        {
            //m_riskTypes.insert(scope->className);
            return true;
        }
    }
    return false;
}

#ifdef USE_GLOG

bool CLog::s_initFlag = false;

#ifndef _WIN32
void error_writer(const char* data, int size)
{
    LOG(INFO) << data;
}
#endif
void CLog::Initialize()
{
    if (!s_initFlag)
    {
        google::InitGoogleLogging("tsc2.log");
        std::string logPath;
        if (CFileDependTable::create_log_directory(&logPath))
            FLAGS_log_dir = logPath.c_str();
        else
            FLAGS_log_dir = ".";

#ifndef _WIN32
        google::InstallFailureSignalHandler();
        google::InstallFailureWriter(error_writer);
#endif
        LOG(INFO) << "glog started.";
        s_initFlag = true;
    }
}

#endif // USE_GLOG

#pragma region Global Statistic Data

CGlobalStatisticData* CGlobalStatisticData::s_instance;
FuncRetInfo FuncRetInfo::UnknownInfo;

std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >& CGlobalStatisticData::get_func_ret_null_thread_data(void* pKey)
{
    TSC_LOCK_ENTER(&m_lock);
    std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >& data = m_threadData[pKey].FuncRetNullInfo;
    TSC_LOCK_LEAVE(&m_lock);
    return data;
}

std::map<const gt::CFunction*, FuncRetStatus>& CGlobalStatisticData::get_func_ret_null_merged_data()
{
    return m_mergedData.FuncRetNullInfo;
}

std::map<std::string, std::list<ArrayIndexInfo> >& CGlobalStatisticData::get_out_of_bounds_thread_data(void* pKey)
{
    TSC_LOCK_ENTER(&m_lock);
    std::map<std::string, std::list<ArrayIndexInfo> >& data = m_threadData[pKey].OutOfBoundsInfo;
    TSC_LOCK_LEAVE(&m_lock);
    return data;
}

void CGlobalStatisticData::merge(bool bDump)
{
    StatisticThreadData tempData;
    merge_internal(tempData, bDump);
    handle_data(tempData, bDump);
}

void CGlobalStatisticData::merge_internal(StatisticThreadData& tempData, bool bDump)
{
    for (std::map<void*, StatisticThreadData >::iterator
        i = m_threadData.begin(), e = m_threadData.end(); i != e; ++i)
    {
        for (std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >::iterator
            i2 = i->second.FuncRetNullInfo.begin(), e2 = i->second.FuncRetNullInfo.end(); i2 != e2; ++i2)
        {
            const std::string& file_name = i2->first;
            std::map<const gt::CFunction*, std::list<FuncRetInfo> >& info = i2->second;

            if (info.empty())
            {
                continue;
            }

            if (tempData.FuncRetNullInfo.count(file_name) > 0)
            {
                continue;
            }

            tempData.FuncRetNullInfo[file_name] = info;
        }

        for (std::map<std::string, std::list<ArrayIndexInfo> >::iterator
            i2 = i->second.OutOfBoundsInfo.begin(), e2 = i->second.OutOfBoundsInfo.end(); i2 != e2; ++i2)
        {
            const std::string& file_name = i2->first;
            std::list<ArrayIndexInfo>& info = i2->second;

            if (info.empty())
            {
                continue;
            }

            if (tempData.OutOfBoundsInfo.count(file_name) > 0)
            {
                continue;
            }

            tempData.OutOfBoundsInfo[file_name] = info;
        }
    }

    if (bDump)
    {
        int index = 0;
        for (std::map<void*, StatisticThreadData >::iterator
            i = m_threadData.begin(), e = m_threadData.end(); i != e; ++i)
        {
            std::stringstream ss;
            ss << index;
            dump_thread_data(i->second, ss.str());
            index++;
        }

        dump_thread_data(tempData, "merged");
    }

    m_threadData.clear();
}

void CGlobalStatisticData::handle_data(StatisticThreadData& tempData, bool bDump)
{
    handle_func_ret_null(tempData);
    handle_out_of_bound(tempData);

    tempData.Clear();

    if (bDump)
    {
        dump_merged_data();
    }
}

void CGlobalStatisticData::handle_func_ret_null(StatisticThreadData &tempData)
{
    for (std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >::iterator
        i2 = tempData.FuncRetNullInfo.begin(), e2 = tempData.FuncRetNullInfo.end(); i2 != e2; ++i2)
    {
        const std::string& fileName = i2->first;
        std::map<const gt::CFunction*, std::list<FuncRetInfo> >& info = i2->second;

        for (std::map<const gt::CFunction*, std::list<FuncRetInfo> >::iterator i3 = info.begin(), e3 = info.end(); i3 != e3; ++i3)
        {
            const gt::CFunction* gtFunc = i3->first;
            std::list<FuncRetInfo>& retList = i3->second;

            for (std::list<FuncRetInfo>::iterator i4 = retList.begin(), e4 = retList.end(); i4 != e4; ++i4)
            {
                FuncRetInfo& ret = *i4;

                if (ret.Op == FuncRetInfo::Use)
                {
                    if (m_mergedData.FuncRetNullInfo[gtFunc].UsedCount < 5)
                    {
                        m_mergedData.FuncRetNullInfo[gtFunc].UsedList.push_back(FuncRetStatus::ErrorMsg(fileName, ret.LineNo, ret.VarName));
                    }
                    ++m_mergedData.FuncRetNullInfo[gtFunc].UsedCount;

                }
                else if (ret.Op == FuncRetInfo::CheckNull)
                {
                    ++m_mergedData.FuncRetNullInfo[gtFunc].CheckNullCount;
                }
            }
        }
    }
}

void CGlobalStatisticData::handle_out_of_bound(StatisticThreadData &tempData)
{
    for (std::map<std::string, std::list<ArrayIndexInfo> >::iterator
        i2 = tempData.OutOfBoundsInfo.begin(), e2 = tempData.OutOfBoundsInfo.end(); i2 != e2; ++i2)
    {
        const std::string& fileName = i2->first;
        std::list<ArrayIndexInfo>& info = i2->second;

        for (std::list<ArrayIndexInfo>::iterator i3 = info.begin(), e3 = info.end(); i3 != e3; ++i3)
        {
            ArrayIndexInfo& aii = *i3;

            StatisticMergedData::Pos pos = { fileName, aii.BoundLine, aii.ArrayLine };
            
            m_mergedData.OutOfBoundsInfo[aii.ArrayStr][aii.BoundStr].push_back(pos);
        }
    }
}

void CGlobalStatisticData::dump_merged_data()
{
    m_mergedData.dump();
}

void CGlobalStatisticData::dump_thread_data(const StatisticThreadData& data, const std::string& file_suffix)
{
    data.dump(file_suffix);
}

void CGlobalStatisticData::report_errors(Settings& setting, std::set<std::string>& errorList)
{
    report_func_ret_null_errors(setting, errorList);
    report_out_of_bounds_errors(setting, errorList);
}

void CGlobalStatisticData::report_func_ret_null_errors(Settings& setting, std::set<std::string>& errorList)
{
    for (std::map<const gt::CFunction*, FuncRetStatus>::const_iterator i = m_mergedData.FuncRetNullInfo.begin(), e = m_mergedData.FuncRetNullInfo.end(); i != e; ++i)
    {
        const std::string& funcName = i->first->GetName();
        const FuncRetStatus& status = i->second;

        if (status.UsedCount <= 3 && status.CheckNullCount >= status.UsedCount * 5)
        {
            for (std::list<FuncRetStatus::ErrorMsg>::const_iterator i2
                = status.UsedList.begin(), e2 = status.UsedList.end(); i2 != e2; ++i2)
            {
                std::stringstream ss;
                ss << "return value of function [" << funcName << "] [" << i2->UsedVarName << "] isn't checked-null, however ["
                    << status.CheckNullCount << "] times checked-null elsewhere.";

                ErrorLogger::ErrorMessage::FileLocation loc(i2->UsedFile, i2->UsedLine);
                std::list<ErrorLogger::ErrorMessage::FileLocation> locList;
                locList.push_back(loc);
                /*ErrorLogger::ErrorMessage msg(locList, Severity::error, ss.str().c_str(), ErrorType::NullPointer, "funcRetNullStatistic", false);

                std::string id = funcName + "|" + I2->UsedVarName;
                msg.SetWebIdentity(id);
                msg.SetFuncInfo(I->first->GetFuncStr());

                std::string errmsg;
                if (setting._xml)
                {
                    if (setting.OpenCodeTrace)
                    {
                        errmsg = msg.toXML_codetrace(setting._verbose, setting._xml_version);
                    }
                    else
                    {
                        errmsg = msg.toXMLEX(setting._verbose, setting._xml_version);
                    }

                }
                else
                    errmsg = msg.toString(setting._verbose);

                errorList.insert(errmsg);*/
            }
        }
    }
}

bool all_upper_case(const std::string& str)
{
    for (int i=0; i<str.length(); ++i)
    {
        char c = str[i];
        if ((c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9'))
        {

        }
        else
            return false;
    }
    return true;
}

bool IsLikeMacro(std::string& str)
{
    std::size_t pos = str.find_last_of("::");
    std::string last_token;
    if (pos != std::string::npos)
    {
        str = str.substr(pos + 1);
    }

    return all_upper_case(str);
}

void CGlobalStatisticData::report_out_of_bounds_errors(Settings& setting, std::set<std::string>& errorList)
{
    for (std::map<std::string, std::map<std::string, std::vector<StatisticMergedData::Pos> > >::const_iterator 
        i = m_mergedData.OutOfBoundsInfo.begin(), e = m_mergedData.OutOfBoundsInfo.end(); i != e; ++i)
    {
        const std::string& arrayName = i->first;

        if (i->second.size() >= 2)
        {
            std::vector<std::string> oneList;
            std::vector<StatisticMergedData::Pos> oneListPos;
            unsigned total_count = 0;
            unsigned right_name_count = 0;
            std::string rightName;
            std::string wrongName;

            for (std::map<std::string, std::vector<StatisticMergedData::Pos> >::const_iterator i2
                = i->second.begin(), e2 = i->second.end(); i2 != e2; ++i2)
            {
                const std::string& boundary = i2->first;
                const std::vector<StatisticMergedData::Pos>& poses = i2->second;

                if (poses.size() == 1)
                {
                    oneList.push_back(boundary);
                    oneListPos.push_back(poses.front());
                }
                else
                {
                    if (poses.size() > right_name_count)
                    {
                        right_name_count = poses.size();
                        rightName = boundary;
                    }
                }
                total_count += poses.size();
            }

            if (oneList.size() == 1 && total_count >= 5)
            {
                // [array] and [array.size()]
                std::string boundary_name = oneList.front();
                std::string right_name = rightName;

                if (boundary_name.find(arrayName)!= std::string::npos)
                {
                    continue;
                }

                bool wrong_like_macro = IsLikeMacro(boundary_name);
                bool right_like_macro = IsLikeMacro(right_name);

                if (wrong_like_macro ^ right_like_macro || boundary_name == right_name)
                {
                    continue;
                }

                std::stringstream ss;
                ss << "The boundary expression [" << oneList.front() << "] may be not right for array [" << arrayName
                    << "] at line [" << oneListPos.front().ArrayLine <<"], cause at least [" 
                    << right_name_count << "] times [" << rightName << "] is used as the array's boundary.";

                ErrorLogger::ErrorMessage::FileLocation loc(oneListPos.front().FilePath, oneListPos.front().BoundaryLine);
                std::list<ErrorLogger::ErrorMessage::FileLocation> locList;
                locList.push_back(loc);
                /*ErrorLogger::ErrorMessage msg(locList, Severity::error, ss.str().c_str(), ErrorType::BufferOverrun, "OutOfBoundsStatistic", false);

                std::string id = oneList.front() + "|" + arrayName;
                msg.SetWebIdentity(id);
                msg.SetFuncInfo("test");

                std::string errmsg;
                if (setting._xml)
                {
                    if (setting.OpenCodeTrace)
                    {
                        errmsg = msg.toXML_codetrace(setting._verbose, setting._xml_version);
                    }
                    else
                    {
                        errmsg = msg.toXMLEX(setting._verbose, setting._xml_version);
                    }

                }
                else
                    errmsg = msg.toString(setting._verbose);
                
                errorList.insert(errmsg);*/
            }
        }
        
    }
}

CGlobalStatisticData* CGlobalStatisticData::Instance()
{
    if (nullptr == s_instance)
    {
        s_instance = new CGlobalStatisticData();
    }
    return s_instance;
}

CGlobalStatisticData::CGlobalStatisticData()
{
    TSC_LOCK_INIT(&m_lock);
}

CGlobalStatisticData::~CGlobalStatisticData()
{
    TSC_LOCK_DELETE(&m_lock);
}

void StatisticThreadData::dump(const std::string& file_suffix) const
{
    dump_func_ret_null(file_suffix);
    dump_out_of_bounds(file_suffix);
}

void StatisticThreadData::dump_func_ret_null(const std::string& file_suffix) const
{
    std::ofstream ofs;
    std::string sPath = CFileDependTable::get_program_directory();
    sPath += "log/stat_funcretnull_thread_" + file_suffix + ".log";
    CFileDependTable::create_log_directory();
    ofs.open(Path::toNativeSeparators(sPath).c_str(), std::ios_base::trunc);

    for (std::map<std::string, std::map<const gt::CFunction*, std::list<FuncRetInfo> > >::const_iterator I = FuncRetNullInfo.begin(), E = FuncRetNullInfo.end(); I != E; ++I)
    {
        ofs << I->first << std::endl;
        for (std::map<const gt::CFunction*, std::list<FuncRetInfo> >::const_iterator I2 = I->second.begin(), E2 = I->second.end(); I2 != E2; ++I2)
        {
            ofs << "\t[" << I2->first->GetFuncStr() << "]" << std::endl;
            for (std::list<FuncRetInfo>::const_iterator I3 = I2->second.begin(), E3 = I2->second.end(); I3 != E3; ++I3)
            {
                std::string sOp;
                if (I3->Op == FuncRetInfo::CheckNull)
                {
                    sOp = "CheckNull";
                }
                else if (I3->Op == FuncRetInfo::Use)
                {
                    sOp = "Use";
                }
                ofs << "\t\t[" << sOp << ", " << I3->LineNo << ", " << I3->VarName << "]" << std::endl;
            }
        }
    }
}

void StatisticThreadData::dump_out_of_bounds(const std::string& file_suffix) const
{
    std::ofstream ofs;
    std::string sPath = CFileDependTable::get_program_directory();
    sPath += "log/stat_outofbounds_thread_" + file_suffix + ".log";
    CFileDependTable::create_log_directory();
    ofs.open(Path::toNativeSeparators(sPath).c_str(), std::ios_base::trunc);

    for (std::map<std::string, std::list<ArrayIndexInfo> >::const_iterator I = OutOfBoundsInfo.begin(), E = OutOfBoundsInfo.end(); I != E; ++I)
    {
        ofs << I->first << std::endl;
        for (std::list<ArrayIndexInfo>::const_iterator I2 = I->second.begin(), E2 = I->second.end(); I2 != E2; ++I2)
        {
            ofs << "\t" << I2->ArrayStr << "|" << I2->ArrayType << "|" << I2->ArrayLine << ", " << I2->BoundStr << "|" << I2->BoundType << "|" << I2->BoundLine << std::endl;
        }
    }
}

void StatisticMergedData::dump()
{
    dump_func_ret_null();
    dump_out_of_bounds();
}

void StatisticMergedData::dump_func_ret_null()
{
    std::ofstream ofs;
    std::string s_path = CFileDependTable::get_program_directory();
    s_path += "log/stat_funcretnull_merged.log";
    CFileDependTable::create_log_directory();
    ofs.open(Path::toNativeSeparators(s_path).c_str(), std::ios_base::trunc);

    for (std::map<const gt::CFunction*, FuncRetStatus>::const_iterator i
        = FuncRetNullInfo.begin(), e = FuncRetNullInfo.end(); i != e; ++i)
    {
        ofs << "[" << i->first->GetFuncStr() << "] \
            CheckNull[" << i->second.CheckNullCount << "] \
            Use[" << i->second.UsedCount << "]" << std::endl;
    }

    ofs.close();
}

void StatisticMergedData::dump_out_of_bounds()
{
    std::ofstream ofs;
    std::string s_path = CFileDependTable::get_program_directory();
    s_path += "log/stat_outofbounds_merged.log";
    CFileDependTable::create_log_directory();
    ofs.open(Path::toNativeSeparators(s_path).c_str(), std::ios_base::trunc);

    for (std::map<std::string, std::map<std::string, std::vector<Pos> > >::const_iterator i
        = OutOfBoundsInfo.begin(), e = OutOfBoundsInfo.end(); i != e; ++i)
    {
        const std::string& arrayName = i->first;
        ofs << arrayName << std::endl;
        for (std::map<std::string, std::vector<Pos> >::const_iterator i2
            = i->second.begin(), e2 = i->second.end();
            i2 != e2; ++i2)
        {
            const std::string& boundaryName = i2->first;
            ofs << "\t" << boundaryName << ", " << i2->second.size() << std::endl;
        }
    }

    ofs.close();
}

#pragma endregion Global Statistic Data

