#include "Parser.h"
#include <stdexcept>
#include <sstream>

// TODO: Full recursive-descent parser implementation
// Stubs below establish the interface; complete each rule in your implementation phase.

// Moves the token vector into the parser; pos_ starts at 0.
Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

// Returns true when pos_ has consumed every token in the stream.
bool Parser::isAtEnd() const { return pos_ >= tokens_.size(); }

// Returns a reference to the token at the current position without advancing.
Token& Parser::current() { return tokens_[pos_]; }

// Returns a reference to the token offset positions ahead; clamps to the last token to avoid out-of-bounds access.
Token& Parser::peek(int offset) {
    size_t idx = pos_ + offset;
    // Clamp to the last token instead of going out of bounds; callers must guard with isAtEnd().
    return idx < tokens_.size() ? tokens_[idx] : tokens_.back();
}

// Returns true if the current token's type equals type without consuming it.
bool Parser::check(TokenType type) const {
    return !isAtEnd() && tokens_[pos_].getType() == type;
}

// Advances pos_ by one and returns true when the current token matches type; otherwise returns false.
bool Parser::match(TokenType type) {
    if (check(type)) { ++pos_; return true; }
    return false;
}

// Advances and returns the current token when it matches expected;
// records a formatted error message and calls synchronize() on mismatch.
Token& Parser::consume(TokenType expected, const std::string& context) {
    if (check(expected)) return tokens_[pos_++];
    std::ostringstream oss;
    oss << "Parse error at line " << current().getLine()
        << ": expected " << tokenTypeToString(expected)
        << " in " << context
        << " but got '" << current().getLexeme() << "'";
    errors_.push_back(oss.str());
    synchronize();
    // Return the last successfully consumed token as a stand-in so callers
    // can continue parsing without dereferencing an invalid position.
    return tokens_[pos_ > 0 ? pos_ - 1 : 0];
}

// Advances pos_ past tokens that cannot start a new statement or declaration,
// stopping at ; } or any statement-opening keyword so parsing can resume.
void Parser::synchronize() {
    // Skip tokens until a likely statement boundary
    while (!isAtEnd()) {
        if (check(TokenType::SEMICOLON))  { ++pos_; return; }
        if (check(TokenType::RBRACE))     { return; }
        if (current().isKeyword()) {
            const auto& lex = current().getLexeme();
            // Stop only at keywords that can open a new statement or declaration.
            // Checking lexemes because there is no "statement-start keyword" category;
            // stopping at every KW_* would also fire on break/continue/case/etc.,
            // causing premature recovery mid-statement.
            if (lex == "if" || lex == "while" || lex == "for" ||
                lex == "return" || lex == "int" || lex == "float" ||
                lex == "double" || lex == "char" || lex == "void")
                return;
        }
        ++pos_;
    }
}

// Delegates to parseProgram() which is the entry point of the grammar.
std::unique_ptr<ParseNode> Parser::parse() {
    return parseProgram();
}

// Repeatedly calls parseDecl() until the token stream is exhausted, building the root node.
std::unique_ptr<ParseNode> Parser::parseProgram() {
    auto node = std::make_unique<ParseNode>("program");
    while (!isAtEnd()) {
        auto decl = parseDecl();
        if (decl) node->addChild(std::move(decl));
    }
    return node;
}

// Stub: will distinguish variable declarations from function declarations by look-ahead.
std::unique_ptr<ParseNode> Parser::parseDecl() {
    if (!isAtEnd()) ++pos_;
    return std::make_unique<ParseNode>("decl");
}

// Stub: will parse "type name = expr ;" producing a var-decl node.
std::unique_ptr<ParseNode> Parser::parseVarDecl(const std::string&, const std::string&, int) {
    // TODO
    return std::make_unique<ParseNode>("var-decl");
}

// Stub: will parse "type name ( params ) compound-stmt" producing a fun-decl node.
std::unique_ptr<ParseNode> Parser::parseFunDecl(const std::string&, const std::string&, int) {
    // TODO
    return std::make_unique<ParseNode>("fun-decl");
}

// Stub: will parse a comma-separated list of typed parameters inside parentheses.
std::unique_ptr<ParseNode> Parser::parseParams() {
    // TODO
    return std::make_unique<ParseNode>("params");
}

// Stub: will parse a single "type name" parameter.
std::unique_ptr<ParseNode> Parser::parseParam() {
    // TODO
    return std::make_unique<ParseNode>("param");
}

// Stub: will parse "{ stmt* }" producing a compound-stmt node.
std::unique_ptr<ParseNode> Parser::parseCompoundStmt() {
    // TODO
    return std::make_unique<ParseNode>("compound-stmt");
}

// Stub: will dispatch to if/while/for/do-while/return/expr statement parsers.
std::unique_ptr<ParseNode> Parser::parseStmt() {
    // TODO
    return std::make_unique<ParseNode>("stmt");
}

// Stub: will parse "if ( expr ) stmt [ else stmt ]".
std::unique_ptr<ParseNode> Parser::parseIfStmt() {
    // TODO
    return std::make_unique<ParseNode>("if-stmt");
}

// Stub: will parse "while ( expr ) stmt".
std::unique_ptr<ParseNode> Parser::parseWhileStmt() {
    // TODO
    return std::make_unique<ParseNode>("while-stmt");
}

// Stub: will parse "do stmt while ( expr ) ;".
std::unique_ptr<ParseNode> Parser::parseDoWhileStmt() {
    // TODO
    return std::make_unique<ParseNode>("do-while-stmt");
}

// Stub: will parse "for ( init ; cond ; incr ) stmt".
std::unique_ptr<ParseNode> Parser::parseForStmt() {
    // TODO
    return std::make_unique<ParseNode>("for-stmt");
}

// Stub: will parse "return [ expr ] ;".
std::unique_ptr<ParseNode> Parser::parseReturnStmt() {
    // TODO
    return std::make_unique<ParseNode>("return-stmt");
}

// Stub: will parse "expr ;".
std::unique_ptr<ParseNode> Parser::parseExprStmt() {
    // TODO
    return std::make_unique<ParseNode>("expr-stmt");
}

// Stub: will parse assignment expressions (lowest precedence binary operator).
std::unique_ptr<ParseNode> Parser::parseExpr() {
    // TODO
    return std::make_unique<ParseNode>("expr");
}

// Stub: will parse logical-OR chains: boolTerm ( "||" boolTerm )*.
std::unique_ptr<ParseNode> Parser::parseBoolExpr() {
    // TODO
    return std::make_unique<ParseNode>("bool-expr");
}

// Stub: will parse logical-AND chains: boolFactor ( "&&" boolFactor )*.
std::unique_ptr<ParseNode> Parser::parseBoolTerm() {
    // TODO
    return std::make_unique<ParseNode>("bool-term");
}

// Stub: will parse "!" relExpr or a bare relational expression.
std::unique_ptr<ParseNode> Parser::parseBoolFactor() {
    // TODO
    return std::make_unique<ParseNode>("bool-factor");
}

// Stub: will parse relational comparisons: arithExpr ( relOp arithExpr )?.
std::unique_ptr<ParseNode> Parser::parseRelExpr() {
    // TODO
    return std::make_unique<ParseNode>("rel-expr");
}

// Stub: will parse additive expressions: term ( ("+" | "-") term )*.
std::unique_ptr<ParseNode> Parser::parseArithExpr() {
    // TODO
    return std::make_unique<ParseNode>("arith-expr");
}

// Stub: will parse multiplicative expressions: factor ( ("*" | "/") factor )*.
std::unique_ptr<ParseNode> Parser::parseTerm() {
    // TODO
    return std::make_unique<ParseNode>("term");
}

// Stub: will parse unary operators and primary expressions (literals, identifiers, calls, parentheses).
std::unique_ptr<ParseNode> Parser::parseFactor() {
    // TODO
    return std::make_unique<ParseNode>("factor");
}

// Stub: will parse "name ( argList )" after the function name has already been consumed.
std::unique_ptr<ParseNode> Parser::parseCall(const std::string&, int) {
    // TODO
    return std::make_unique<ParseNode>("call");
}

// Stub: will parse a comma-separated list of actual argument expressions between parentheses.
std::unique_ptr<ParseNode> Parser::parseArgList() {
    // TODO
    return std::make_unique<ParseNode>("arg-list");
}
