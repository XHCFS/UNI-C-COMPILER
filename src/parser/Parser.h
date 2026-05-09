#pragma once
#include "Token.h"
#include "ParseTree.h"
#include <vector>
#include <memory>
#include <string>

// Recursive-descent parser that converts a token stream into a parse tree.
// Grammar errors are collected into an error list; the parser attempts to
// recover and continue via synchronize() rather than aborting immediately.
class Parser {
public:
    // Constructs the parser with the token list produced by the Lexer.
    explicit Parser(std::vector<Token> tokens);

    // Starts parsing from the top-level rule and returns the completed parse tree.
    std::unique_ptr<ParseNode> parse();

    // Returns the list of parse error messages collected during parse().
    const std::vector<std::string>& errors() const { return errors_; }

private:
    std::vector<Token>        tokens_;  // Token stream consumed left-to-right by the parser.
    size_t                    pos_ = 0; // Index of the current token being examined.
    std::vector<std::string>  errors_;  // Accumulated parse error messages.

    // Returns true if pos_ has reached the end of the token stream.
    bool    isAtEnd() const;

    // Returns a reference to the token at the current position.
    Token&  current();

    // Returns a reference to the token offset positions ahead of the current one.
    Token&  peek(int offset = 1);

    // Consumes the current token if it matches expected; records an error and synchronizes otherwise.
    Token&  consume(TokenType expected, const std::string& context);

    // Returns true if the current token's type equals type without consuming it.
    bool    check(TokenType type) const;

    // Consumes and returns true if the current token matches type; returns false otherwise.
    bool    match(TokenType type);

    // Skips tokens until a likely statement boundary to recover from a parse error.
    void    synchronize();

    // --- Grammar rules — each returns a subtree for the matched construct ---

    // Parses the top-level program as a sequence of declarations.
    std::unique_ptr<ParseNode> parseProgram();

    // Parses a single top-level declaration (variable or function).
    std::unique_ptr<ParseNode> parseDecl();

    // Parses a variable declaration given its already-consumed type, name, and line.
    std::unique_ptr<ParseNode> parseVarDecl(const std::string& type, const std::string& name, int line);

    // Parses a function declaration given its already-consumed type, name, and line.
    std::unique_ptr<ParseNode> parseFunDecl(const std::string& type, const std::string& name, int line);

    // Parses a comma-separated formal parameter list between parentheses.
    std::unique_ptr<ParseNode> parseParams();

    // Parses a single formal parameter (type + name).
    std::unique_ptr<ParseNode> parseParam();

    // Parses a brace-enclosed block of statements.
    std::unique_ptr<ParseNode> parseCompoundStmt();

    // Dispatches to the appropriate statement parser based on the current token.
    std::unique_ptr<ParseNode> parseStmt();

    // Parses an if statement, including an optional else branch.
    std::unique_ptr<ParseNode> parseIfStmt();

    // Parses a while loop.
    std::unique_ptr<ParseNode> parseWhileStmt();

    // Parses a do-while loop.
    std::unique_ptr<ParseNode> parseDoWhileStmt();

    // Parses a for loop with init, condition, and increment clauses.
    std::unique_ptr<ParseNode> parseForStmt();

    // Parses a return statement with an optional expression.
    std::unique_ptr<ParseNode> parseReturnStmt();

    // Parses an expression followed by a semicolon.
    std::unique_ptr<ParseNode> parseExprStmt();

    // Parses an assignment expression (lowest-precedence binary operator).
    std::unique_ptr<ParseNode> parseExpr();

    // Parses a logical-OR expression (||).
    std::unique_ptr<ParseNode> parseBoolExpr();

    // Parses a logical-AND expression (&&).
    std::unique_ptr<ParseNode> parseBoolTerm();

    // Parses a logical-NOT expression (!).
    std::unique_ptr<ParseNode> parseBoolFactor();

    // Parses a relational expression (==, !=, <, >, <=, >=).
    std::unique_ptr<ParseNode> parseRelExpr();

    // Parses an additive expression (+ and -).
    std::unique_ptr<ParseNode> parseArithExpr();

    // Parses a multiplicative expression (* and /).
    std::unique_ptr<ParseNode> parseTerm();

    // Parses a unary expression or primary (literal, identifier, parenthesised expression).
    std::unique_ptr<ParseNode> parseFactor();

    // Parses a function call given the already-consumed function name and its line.
    std::unique_ptr<ParseNode> parseCall(const std::string& name, int line);

    // Parses a comma-separated list of actual arguments between parentheses.
    std::unique_ptr<ParseNode> parseArgList();
};
