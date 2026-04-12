#include "SymbolTable.h"

SymbolTable::SymbolTable() = default;

bool SymbolTable::insert(const Token& token) {
    const std::string& name = token.getLexeme();
    if (entries_.count(name)) return false;
    entries_.emplace(name, SymbolEntry(token));
    return true;
}

bool SymbolTable::exists(const std::string& name) const {
    return entries_.count(name) > 0;
}

SymbolEntry* SymbolTable::find(const std::string& name) {
    auto it = entries_.find(name);
    return it != entries_.end() ? &it->second : nullptr;
}

const std::unordered_map<std::string, SymbolEntry>& SymbolTable::getAllEntries() const {
    return entries_;
}

void SymbolTable::clear() {
    entries_.clear();
}
