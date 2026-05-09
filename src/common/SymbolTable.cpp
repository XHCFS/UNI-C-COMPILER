#include "SymbolTable.h"
using namespace std;

// Default constructor — entries_ starts empty.
SymbolTable::SymbolTable() = default;

// Inserts a new SymbolEntry at the (scope, name) key. The default scope of -1
// keeps the existing Lexer::scanIdentifierOrKeyword() call site working without
// modification. Returns false when the same (scope, name) already exists.
bool SymbolTable::insert(const Token& token, int scope) {
    Key key{scope, token.getLexeme()};
    if (entries_.count(key)) return false;
    SymbolEntry entry(token);
    entry.setScope(scope);
    entries_.emplace(key, entry);
    return true;
}

// Parser-side declaration write. Inserts a new entry at (scope, name) with the
// supplied dataType and the matching scope on the entry. Returns false (without
// inserting) on same-(scope, name) collision so the parser can report a
// redeclaration error.
bool SymbolTable::declare(const Token& token, int scope, const string& dataType) {
    Key key{scope, token.getLexeme()};
    if (entries_.count(key)) return false;
    SymbolEntry entry(token);
    entry.setScope(scope);
    entry.setDataType(dataType);
    entries_.emplace(key, entry);
    return true;
}

// Exact-scope existence check; safe to call on a const table.
bool SymbolTable::exists(const string& name, int scope) const {
    Key key{scope, name};
    return entries_.count(key) > 0;
}

// Exact-scope lookup; returns mutable pointer for in-place updates by callers.
SymbolEntry* SymbolTable::find(const string& name, int scope) {
    Key key{scope, name};
    auto it = entries_.find(key);
    return it != entries_.end() ? &it->second : nullptr;
}

// Walks activeScopes from innermost (back) to outermost (front); the lexer's
// scope=-1 lane is intentionally never visited because callers supply only
// real scope IDs (>= 0).
SymbolEntry* SymbolTable::lookup(const string& name, const vector<int>& activeScopes) {
    for (auto it = activeScopes.rbegin(); it != activeScopes.rend(); ++it) {
        if (auto* e = find(name, *it)) return e;
    }
    return nullptr;
}

// Exposes the raw map for read-only iteration; the GUI filters by key.scope.
const unordered_map<SymbolTable::Key, SymbolEntry, SymbolTable::KeyHash>&
SymbolTable::getAllEntries() const {
    return entries_;
}

// Removes all entries; called before re-running on new input.
void SymbolTable::clear() {
    entries_.clear();
}
