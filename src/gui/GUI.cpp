#include "GUI.h"
#include <iostream>

// TODO: Replace terminal output with actual GUI implementation.

GUI::GUI() {}

void GUI::run() {
    std::cout << "C Compiler - Lexer & Parser\n";
    std::cout << "GUI not yet implemented. See GUI.h for interface.\n";
}

void GUI::showLexerOutput(const std::vector<Token>& tokens,
                          const SymbolTable& table,
                          const std::vector<std::string>& errors) {
    std::cout << "\n--- Tokens ---\n";
    for (const auto& tok : tokens) {
        std::cout << "[" << tokenTypeToString(tok.type) << "] "
                  << tok.lexeme << "  (line " << tok.line << ")\n";
    }
    table.print();
    if (!errors.empty()) {
        std::cout << "--- Lexer Errors ---\n";
        for (const auto& e : errors) std::cout << e << "\n";
    }
}

void GUI::showParserOutput(const ParseNode* tree,
                           const std::vector<std::string>& errors) {
    std::cout << "\n--- Parse Tree ---\n";
    printParseTree(tree);
    if (!errors.empty()) {
        std::cout << "--- Parser Errors ---\n";
        for (const auto& e : errors) std::cout << e << "\n";
    }
}
