#pragma once
#include "SymbolEntry.h"
#include <unordered_map>
#include <vector>
#include <string>
using namespace std;


// Shared symbol table for both the lexer and the parser, keyed on the composite
// (scope, name). Three lanes coexist:
//   scope = -1 : lexer-seen identifiers (one entry per unique name)
//   scope =  0 : parser file-scope declarations
//   scope >= 1 : parser nested-scope declarations (one fresh ID per '{')
class SymbolTable {
public:
    // Composite key combining scope and name. Two declarations of the same
    // name in different scopes coexist as two distinct entries.
    struct Key {
        int    scope;
        string name;
        bool operator==(const Key& other) const noexcept {
            return scope == other.scope && name == other.name;
        }
    };

    // Hash combiner over (scope, name); standard boost-style mix.
    struct KeyHash {
        size_t operator()(const Key& k) const noexcept {
            size_t h1 = hash<int>{}(k.scope);
            size_t h2 = hash<string>{}(k.name);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    // Constructs an empty symbol table.
    SymbolTable();

    // Lexer-side insert (default scope = -1 keeps Lexer::scanIdentifierOrKeyword()
    // unchanged). Inserts a new entry keyed on (scope, token.getLexeme()) and
    // returns true; returns false on (scope, name) collision without inserting.
    bool insert(const Token& token, int scope = -1);

    // Parser-side declaration write. Inserts a new entry at (scope, name) with
    // dataType_ and scope_ populated. Returns false (and does not insert) when
    // the same (scope, name) is already present — i.e. same-scope redeclaration.
    bool declare(const Token& token, int scope, const string& dataType);

    // Returns true if (scope, name) is present.
    bool exists(const string& name, int scope) const;

    // Returns a mutable pointer to the entry at (scope, name), or nullptr.
    SymbolEntry* find(const string& name, int scope);

    // Shadowing-aware lookup: walks activeScopes innermost-to-outermost (back
    // to front), returning the first matching entry. The scope=-1 lane is
    // intentionally never visited — callers pass only real scope IDs (>= 0).
    SymbolEntry* lookup(const string& name, const vector<int>& activeScopes);

    // Read-only view of every entry, used by the GUI (filters by key.scope).
    const unordered_map<Key, SymbolEntry, KeyHash>& getAllEntries() const;

    // Removes all entries; called before re-running on new input.
    void clear();

private:
    unordered_map<Key, SymbolEntry, KeyHash> entries_;
};
