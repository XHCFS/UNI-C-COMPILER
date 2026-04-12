#include "SymbolEntry.h"

SymbolEntry::SymbolEntry(const Token& token)
    : token_(token), dataType_(), scope_(-1) {}

const std::string& SymbolEntry::getName()  const { return token_.getLexeme(); }
int                SymbolEntry::getLine()  const { return token_.getLine(); }
const Token&       SymbolEntry::getToken() const { return token_; }

void               SymbolEntry::setDataType(const std::string& type) { dataType_ = type; }
const std::string& SymbolEntry::getDataType() const                  { return dataType_; }

void               SymbolEntry::setScope(int scopeLevel) { scope_ = scopeLevel; }
int                SymbolEntry::getScope() const          { return scope_; }
