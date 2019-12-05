#ifndef BCACPP_TOKENEX_H
#define BCACPP_TOKENEX_H

#include <string>
#include <vector>

class Token;
class Variable;
class Scope;

namespace gt
{
    class CFunction;
    class CField;
};

struct TokenSection
{
    const Token* Begin;
    const Token* End;

    static TokenSection EmptySection;

    TokenSection() : Begin(nullptr), End(nullptr)
    {}
    TokenSection(const Token* begin, const Token* end)
        : Begin(begin), End(end)
    {}

    bool Empty() const
    {
        return (Begin == nullptr) || (End == nullptr);
    }

    void Clear()
    {
        Begin = nullptr;
        End = nullptr;
    }

    const Token* last_token() const;
    bool is_one_token() const;

    std::string to_type_string(const char* sep = nullptr) const;
};

struct SExpr
{
    // not used now, may be used in the future though, just keep it here
    enum ExprType
    {
        None,
        Variable,
        Function,
        Array,
    };

    unsigned VarId;
    std::string ExprStr;

    SExpr() : VarId(0)
    {

    }

    SExpr(unsigned varid, const std::string& str) : VarId(varid), ExprStr(str)
    {

    }

    bool operator<(const SExpr& other) const
    {
        if (VarId != other.VarId)
        {
            return VarId < other.VarId;
        }
        else
        {
            if (VarId != 0)
            {
                return false;
            }
            else
            {
                return ExprStr < other.ExprStr;
            }
        }
    }

    bool operator == (const SExpr& other) const
    {
        return !(*this < other) && !(other < *this);
    }

    bool operator != (const SExpr& other) const
    {
        return !(*this == other);
    }
};

struct SExprLocation : public TokenSection
{
    static SExprLocation EmptyExprLoc;

    SExprLocation() : TokenSection(), TokTop(nullptr)
    {
    }

    bool Empty() const { return TokTop == nullptr; }

    SExprLocation(const Token* begin, const Token* end, const Token* tokTop, unsigned varid) :
        TokenSection(begin, end), TokTop(tokTop), Expr(varid, to_type_string())
    {

    }

    bool operator == (const SExprLocation& other) const
    {
        return TokTop == other.TokTop;
    }

    bool operator != (const SExprLocation& other) const
    {
        return TokTop != other.TokTop;
    }
    
    const Variable* get_variable() const;

public:
    const Token* TokTop;
    SExpr Expr;
};

class TokenEx
{
public:
    TokenEx()
        : m_gtFunc(nullptr)
    {}

    void SetGtFunc(const gt::CFunction* gtFunc) { m_gtFunc = gtFunc; }
    const gt::CFunction* GetGtFunc() const { return m_gtFunc; }
    
private:
    const gt::CFunction* m_gtFunc;
};

////tsc
////bool iscast(const Token *tok);
bool is_cast(const Token *tok);

SExprLocation create_sexpr_by_expr_tok(const Token* tok);

unsigned get_standard_type_size(const Token* typeStartTok, const Token* typeEndTok);
unsigned get_standard_type_size(const gt::CField* gtField);

void get_params_by_ast(const Token* tok, std::vector<const Token*> &params);

int try_get_expr_size(const Token* tok);

#endif // BCACPP_TOKENEX_H
