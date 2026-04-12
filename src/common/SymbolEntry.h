#pragma once
#include "Token.h"
#include <string>

class SymbolEntry {
public:
    explicit SymbolEntry(const Token& token);

    // Lexer-owned fields — derived from the stored token.
    const std::string& getName()  const;   // token.getLexeme()
    int                getLine()  const;   // token.getLine()
    const Token&       getToken() const;

    // Parser-owned fields — Not used now.
    void               setDataType(const std::string& type);
    const std::string& getDataType() const;
    void               setScope(int scopeLevel);
    int                getScope() const;

private:
    Token       token_;
    std::string dataType_;
    int         scope_;
};
