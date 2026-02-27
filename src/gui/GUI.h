#pragma once
#include "Token.h"
#include "SymbolTable.h"
#include "ParseTree.h"
#include <vector>
#include <string>

// GUI interface — replace with actual GUI library (Qt, wxWidgets, ImGui, etc.)
// in the implementation phase. This stub provides the interface contract.
class GUI {
public:
    GUI();
    void run();

private:
    void showLexerOutput(const std::vector<Token>& tokens,
                         const SymbolTable& table,
                         const std::vector<std::string>& errors);

    void showParserOutput(const ParseNode* tree,
                          const std::vector<std::string>& errors);
};
