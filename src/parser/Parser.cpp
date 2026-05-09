#include "Parser.h"
using namespace std;

// Binds the token vector to a TokenStream, stores the symbol-table reference,
// and seeds scope tracking with file scope (id 0).
Parser::Parser(const vector<Token>& tokens, SymbolTable& symbolTable)
    : input_(tokens), symbolTable_(symbolTable),
      nextScopeId_(0), scopeStack_{0} {}

// Returns the accumulated parse-error list.
const vector<ParseError>& Parser::getErrors() const { return errors_; }

// Appends a new ParseError to errors_; mirrors Lexer::addError.
void Parser::addError(const string& message, int line, int column,
                      const string& text) {
    errors_.emplace_back(message, line, column, text);
}

// Match-and-consume with diagnostic on miss. On miss, reports an error using
// the CURRENT (unchanged) cursor's line/column/lexeme so the message points at
// the token the parser actually saw.
bool Parser::expect(TokenType type, const string& message) {
    if (input_.match(type)) return true;
    const Token& bad = input_.peek();
    addError(message, bad.getLine(), bad.getColumn(), bad.getLexeme());
    return false;
}

// Consume tokens until the cursor sits at a synchronisation point. SEMICOLON
// is itself consumed (we want to resume past the broken statement). Brace and
// statement-leading keywords are left in place so the next grammar method can
// decide how to handle them.
void Parser::synchronize() {
    while (!input_.eof()) {
        TokenType t = input_.peek().getType();
        switch (t) {
            case TokenType::SEMICOLON:
                input_.get();   // consume and stop
                return;
            case TokenType::LBRACE:
            case TokenType::RBRACE:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_DO:
            case TokenType::KW_RETURN:
            case TokenType::KW_BREAK:
            case TokenType::KW_CONTINUE:
                return;         // leave for the caller
            default:
                input_.get();   // skip and keep looking
                break;
        }
    }
}
