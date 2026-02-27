#pragma once
#include "Token.h"
#include "ParseTree.h"
#include <vector>
#include <memory>
#include <string>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<ParseNode> parse();
    const std::vector<std::string>& errors() const { return errors_; }

private:
    std::vector<Token> tokens_;
    size_t             pos_ = 0;
    std::vector<std::string> errors_;

    Token&  current();
    Token&  peek(int offset = 1);
    Token&  consume(TokenType expected, const std::string& context);
    bool    check(TokenType type) const;
    bool    match(TokenType type);
    void    synchronize();

    // Grammar rules — each returns a parse node
    std::unique_ptr<ParseNode> parseProgram();
    std::unique_ptr<ParseNode> parseDecl();
    std::unique_ptr<ParseNode> parseVarDecl(const std::string& type, const std::string& name, int line);
    std::unique_ptr<ParseNode> parseFunDecl(const std::string& type, const std::string& name, int line);
    std::unique_ptr<ParseNode> parseParams();
    std::unique_ptr<ParseNode> parseParam();
    std::unique_ptr<ParseNode> parseCompoundStmt();
    std::unique_ptr<ParseNode> parseStmt();
    std::unique_ptr<ParseNode> parseIfStmt();
    std::unique_ptr<ParseNode> parseWhileStmt();
    std::unique_ptr<ParseNode> parseDoWhileStmt();
    std::unique_ptr<ParseNode> parseForStmt();
    std::unique_ptr<ParseNode> parseReturnStmt();
    std::unique_ptr<ParseNode> parseExprStmt();
    std::unique_ptr<ParseNode> parseExpr();
    std::unique_ptr<ParseNode> parseBoolExpr();
    std::unique_ptr<ParseNode> parseBoolTerm();
    std::unique_ptr<ParseNode> parseBoolFactor();
    std::unique_ptr<ParseNode> parseRelExpr();
    std::unique_ptr<ParseNode> parseArithExpr();
    std::unique_ptr<ParseNode> parseTerm();
    std::unique_ptr<ParseNode> parseFactor();
    std::unique_ptr<ParseNode> parseCall(const std::string& name, int line);
    std::unique_ptr<ParseNode> parseArgList();
};
