#include "SymbolEntry.h"
using namespace std;
// Stores the token and initialises parser-phase fields to their default empty values.
SymbolEntry::SymbolEntry(const Token& token)
    : token_(token), dataType_(), scope_(-1) {}

// Delegates name lookup to the stored token's lexeme.
const string& SymbolEntry::getName() const { return token_.getLexeme(); }

// Delegates line lookup to the stored token's source location.
int SymbolEntry::getLine() const { return token_.getLine(); }

// Returns the original token as stored by the lexer.
const Token& SymbolEntry::getToken() const { return token_; }


// Stores the C type string resolved by the parser (e.g. "int", "float*").
void SymbolEntry::setDataType(const string& type) { dataType_ = type; }

// Returns the C type string, or empty string if the parser has not yet set it.
const string& SymbolEntry::getDataType() const { return dataType_; }

// Records the lexical scope depth at which this identifier was declared.
void SymbolEntry::setScope(int scopeLevel) { scope_ = scopeLevel; }

// Returns the scope depth, or -1 if not yet assigned by the parser.
int SymbolEntry::getScope() const { return scope_; }
