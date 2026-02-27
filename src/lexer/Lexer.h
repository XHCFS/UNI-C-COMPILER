#pragma once
#include "Token.h"
#include "SymbolTable.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();
    const SymbolTable& symbolTable() const { return symbolTable_; }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    std::string source_;
    size_t      pos_    = 0;
    int         line_   = 1;
    int         column_ = 1;
    SymbolTable symbolTable_;
    std::vector<std::string> errors_;

    char        current() const;
    char        peek(int offset = 1) const;
    char        advance();
    void        skipWhitespace();
    void        skipLineComment();
    void        skipBlockComment();

    Token       readIdentifierOrKeyword();
    Token       readNumber();
    Token       readCharLiteral();
    Token       readStringLiteral();
    Token       readOperatorOrPunct();

    bool        isKeyword(const std::string& s) const;
};
