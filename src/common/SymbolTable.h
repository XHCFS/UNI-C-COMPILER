#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

enum class SymbolKind { VARIABLE, FUNCTION, PARAMETER };

struct Symbol {
    std::string name;
    std::string dataType;
    SymbolKind  kind;
    int         scopeLevel;
    int         lineNumber;
};

class SymbolTable {
public:
    void enterScope();
    void exitScope();

    bool   insert(const Symbol& symbol);
    std::optional<Symbol> lookup(const std::string& name) const;

    void print() const;
    int  currentScope() const { return scopeLevel_; }

private:
    // Each scope is a map from name -> Symbol
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    int scopeLevel_ = 0;
};
