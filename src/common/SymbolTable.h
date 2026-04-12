#pragma once
#include "SymbolEntry.h"
#include <unordered_map>
#include <string>

class SymbolTable {
public:
    SymbolTable();

    // Insert a new identifier entry keyed on token.getLexeme().
    // Returns true on success, false if the name is already present.
    bool insert(const Token& token);

    bool          exists(const std::string& name) const;
    SymbolEntry*  find(const std::string& name);
    const std::unordered_map<std::string, SymbolEntry>& getAllEntries() const;

    void clear();

private:
    std::unordered_map<std::string, SymbolEntry> entries_;
};
