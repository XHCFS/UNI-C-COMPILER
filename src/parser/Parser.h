#pragma once
#include "TokenStream.h"
#include "ParseError.h"
#include "Token.h"
#include "SymbolTable.h"
#include <vector>
#include <string>
using namespace std;


// The main syntactic analyzer. Consumes the lexer's token stream, builds an
// AST (added in a later phase), writes declared identifiers into the shared
// SymbolTable via declare(), and records parse errors.
//
// This file currently exposes the constructor, error accessors, and the
// private helper layer (addError/expect/synchronize) that grammar methods
// will sit on top of. Grammar methods and parse() are added in a later phase.
class Parser {
public:
    // Binds the token vector to a TokenStream, stores the symbol-table
    // reference, and initialises scope tracking (file scope = 0 is preloaded).
    Parser(const vector<Token>& tokens, SymbolTable& symbolTable);

    // Returns the list of errors accumulated so far. Same shape as Lexer::getErrors().
    const vector<ParseError>& getErrors() const;

private:
    TokenStream  input_;          // token-level cursor over the lexer's output
    SymbolTable& symbolTable_;    // shared symbol table; written via declare()
    vector<ParseError> errors_;   // accumulated parse errors

    // Scope tracking for shadowing-aware declare/lookup against SymbolTable.
    // nextScopeId_ is monotonic; scopeStack_ holds the active scope chain
    // (innermost at the back). File scope occupies 0 and is always present.
    int         nextScopeId_;
    vector<int> scopeStack_;

    // Append a new ParseError to errors_. Same signature as Lexer::addError.
    void addError(const string& message, int line, int column,
                  const string& text = "");

    // If the next token's type equals expected, consume it and return true.
    // Otherwise, record an error at the current cursor (using peek() for
    // line/column/lexeme) and return false WITHOUT consuming.
    bool expect(TokenType type, const string& message);

    // Recover after a parse failure: consume tokens until the cursor sits at
    // a synchronisation point — a SEMICOLON (consumed), a brace boundary
    // (LBRACE/RBRACE, left for the caller), a statement-leading keyword
    // (KW_IF/WHILE/FOR/DO/RETURN/BREAK/CONTINUE, left for the caller), or EOF.
    void synchronize();
};
