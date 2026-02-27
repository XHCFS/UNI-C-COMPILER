#include "Parser.h"
#include <stdexcept>
#include <sstream>

// TODO: Full recursive-descent parser implementation
// Stubs below establish the interface; complete each rule in your implementation phase.

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

Token& Parser::current() { return tokens_[pos_]; }
Token& Parser::peek(int offset) {
    size_t idx = pos_ + offset;
    return idx < tokens_.size() ? tokens_[idx] : tokens_.back();
}

bool Parser::check(TokenType type) const { return tokens_[pos_].type == type; }

bool Parser::match(TokenType type) {
    if (check(type)) { ++pos_; return true; }
    return false;
}

Token& Parser::consume(TokenType expected, const std::string& context) {
    if (check(expected)) return tokens_[pos_++];
    std::ostringstream oss;
    oss << "Parse error at line " << current().line
        << ": expected " << tokenTypeToString(expected)
        << " in " << context
        << " but got '" << current().lexeme << "'";
    errors_.push_back(oss.str());
    synchronize();
    return tokens_[pos_ > 0 ? pos_ - 1 : 0];
}

void Parser::synchronize() {
    // Skip tokens until a likely statement boundary
    while (!check(TokenType::END_OF_FILE)) {
        if (check(TokenType::SEMICOLON))  { ++pos_; return; }
        if (check(TokenType::RBRACE))     { return; }
        if (current().type == TokenType::KEYWORD) {
            const auto& lex = current().lexeme;
            if (lex == "if" || lex == "while" || lex == "for" ||
                lex == "return" || lex == "int" || lex == "float" ||
                lex == "double" || lex == "char" || lex == "void")
                return;
        }
        ++pos_;
    }
}

std::unique_ptr<ParseNode> Parser::parse() {
    return parseProgram();
}

std::unique_ptr<ParseNode> Parser::parseProgram() {
    auto node = std::make_unique<ParseNode>("program");
    while (!check(TokenType::END_OF_FILE)) {
        auto decl = parseDecl();
        if (decl) node->addChild(std::move(decl));
    }
    return node;
}

std::unique_ptr<ParseNode> Parser::parseDecl() {
    // TODO: implement full declaration parsing
    auto node = std::make_unique<ParseNode>("decl");
    return node;
}

std::unique_ptr<ParseNode> Parser::parseVarDecl(const std::string&, const std::string&, int) {
    // TODO
    return std::make_unique<ParseNode>("var-decl");
}

std::unique_ptr<ParseNode> Parser::parseFunDecl(const std::string&, const std::string&, int) {
    // TODO
    return std::make_unique<ParseNode>("fun-decl");
}

std::unique_ptr<ParseNode> Parser::parseParams() {
    // TODO
    return std::make_unique<ParseNode>("params");
}

std::unique_ptr<ParseNode> Parser::parseParam() {
    // TODO
    return std::make_unique<ParseNode>("param");
}

std::unique_ptr<ParseNode> Parser::parseCompoundStmt() {
    // TODO
    return std::make_unique<ParseNode>("compound-stmt");
}

std::unique_ptr<ParseNode> Parser::parseStmt() {
    // TODO
    return std::make_unique<ParseNode>("stmt");
}

std::unique_ptr<ParseNode> Parser::parseIfStmt() {
    // TODO
    return std::make_unique<ParseNode>("if-stmt");
}

std::unique_ptr<ParseNode> Parser::parseWhileStmt() {
    // TODO
    return std::make_unique<ParseNode>("while-stmt");
}

std::unique_ptr<ParseNode> Parser::parseDoWhileStmt() {
    // TODO
    return std::make_unique<ParseNode>("do-while-stmt");
}

std::unique_ptr<ParseNode> Parser::parseForStmt() {
    // TODO
    return std::make_unique<ParseNode>("for-stmt");
}

std::unique_ptr<ParseNode> Parser::parseReturnStmt() {
    // TODO
    return std::make_unique<ParseNode>("return-stmt");
}

std::unique_ptr<ParseNode> Parser::parseExprStmt() {
    // TODO
    return std::make_unique<ParseNode>("expr-stmt");
}

std::unique_ptr<ParseNode> Parser::parseExpr() {
    // TODO
    return std::make_unique<ParseNode>("expr");
}

std::unique_ptr<ParseNode> Parser::parseBoolExpr() {
    // TODO
    return std::make_unique<ParseNode>("bool-expr");
}

std::unique_ptr<ParseNode> Parser::parseBoolTerm() {
    // TODO
    return std::make_unique<ParseNode>("bool-term");
}

std::unique_ptr<ParseNode> Parser::parseBoolFactor() {
    // TODO
    return std::make_unique<ParseNode>("bool-factor");
}

std::unique_ptr<ParseNode> Parser::parseRelExpr() {
    // TODO
    return std::make_unique<ParseNode>("rel-expr");
}

std::unique_ptr<ParseNode> Parser::parseArithExpr() {
    // TODO
    return std::make_unique<ParseNode>("arith-expr");
}

std::unique_ptr<ParseNode> Parser::parseTerm() {
    // TODO
    return std::make_unique<ParseNode>("term");
}

std::unique_ptr<ParseNode> Parser::parseFactor() {
    // TODO
    return std::make_unique<ParseNode>("factor");
}

std::unique_ptr<ParseNode> Parser::parseCall(const std::string&, int) {
    // TODO
    return std::make_unique<ParseNode>("call");
}

std::unique_ptr<ParseNode> Parser::parseArgList() {
    // TODO
    return std::make_unique<ParseNode>("arg-list");
}
