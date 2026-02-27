#include "SymbolTable.h"
#include <iostream>

void SymbolTable::enterScope() {
    scopes_.emplace_back();
    ++scopeLevel_;
}

void SymbolTable::exitScope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
        --scopeLevel_;
    }
}

bool SymbolTable::insert(const Symbol& symbol) {
    if (scopes_.empty()) enterScope();
    auto& current = scopes_.back();
    if (current.count(symbol.name)) return false; // already declared in this scope
    current[symbol.name] = symbol;
    return true;
}

std::optional<Symbol> SymbolTable::lookup(const std::string& name) const {
    // Search from innermost to outermost scope
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return std::nullopt;
}

void SymbolTable::print() const {
    std::cout << "\n=== Symbol Table ===\n";
    std::cout << std::left;
    std::cout << "Name             Type        Kind        Scope  Line\n";
    std::cout << std::string(55, '-') << "\n";
    for (const auto& scope : scopes_) {
        for (const auto& [name, sym] : scope) {
            std::string kindStr;
            switch (sym.kind) {
                case SymbolKind::VARIABLE:  kindStr = "variable";  break;
                case SymbolKind::FUNCTION:  kindStr = "function";  break;
                case SymbolKind::PARAMETER: kindStr = "parameter"; break;
            }
            printf("%-16s %-11s %-11s %-6d %d\n",
                sym.name.c_str(), sym.dataType.c_str(),
                kindStr.c_str(), sym.scopeLevel, sym.lineNumber);
        }
    }
    std::cout << "====================\n\n";
}
