#include "SymbolTable.h"
using namespace std;

// Default constructor — entries_ starts empty.
SymbolTable::SymbolTable() = default;

// Inserts a new SymbolEntry keyed on the token's lexeme.
// Scope resolution is intentionally left to the parser phase.
// Returns false without inserting if the name already exists.
bool SymbolTable::insert(const Token& token) {
    const string& name = token.getLexeme();
    if (entries_.count(name)) return false;
    entries_.emplace(name, SymbolEntry(token));
    return true;
}

// Checks existence without returning a pointer; safe to call on a const table.
bool SymbolTable::exists(const string& name) const {
    return entries_.count(name) > 0;
}

// Returns a mutable pointer so the parser can update dataType and scope in-place.
SymbolEntry* SymbolTable::find(const string& name) {
    auto it = entries_.find(name);
    return it != entries_.end() ? &it->second : nullptr;
}

// Exposes the raw map for read-only iteration (used by the GUI to populate the symbol-table tab).
const unordered_map<string, SymbolEntry>& SymbolTable::getAllEntries() const {
    return entries_;
}

// Removes all entries; called before re-running the lexer on new input.
void SymbolTable::clear() {
    entries_.clear();
}
