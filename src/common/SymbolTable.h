#pragma once
#include "SymbolEntry.h"
#include <unordered_map>
#include <string>
using namespace std;

// Flat hash-map of every user-defined identifier seen during lexing.
// Scope resolution is deferred to the parser phase.
class SymbolTable {
public:
    // Constructs an empty symbol table.
    SymbolTable();

    // Inserts a new entry keyed on token.getLexeme().
    // Returns true on success, false if the name is already present.
    bool insert(const Token& token);

    // Returns true if an entry with the given name exists in the table.
    bool exists(const string& name) const;

    // Returns a pointer to the entry for the given name, or nullptr if not found.
    SymbolEntry*  find(const string& name);

    // Returns a read-only view of all entries for iteration (e.g. populating the GUI table).
    const unordered_map<string, SymbolEntry>& getAllEntries() const;

    // Removes all entries from the table.
    void clear();

private:
    // Maps each identifier name to its SymbolEntry.
    unordered_map<string, SymbolEntry> entries_;
};
