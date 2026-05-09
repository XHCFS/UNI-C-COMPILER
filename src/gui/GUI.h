#pragma once
#include "Token.h"
#include "SymbolTable.h"
#include "Ast.h"
#include <vector>
#include <string>
using namespace std;
// Legacy stub GUI — superseded by the Qt MainWindow implementation.
// Kept as an interface contract; replace method bodies with real GUI calls
// when integrating a different frontend.
class GUI {
public:
    // Constructs the GUI object (currently a no-op stub).
    GUI();

    // Entry point that launches the GUI loop (currently prints a placeholder message).
    void run();

private:
    // Displays the lexer output — tokens, symbol table entries, and errors — to the UI.
    void showLexerOutput(const vector<Token>& tokens,
                         const SymbolTable& table,
                         const vector<string>& errors);

    // Displays the parser output — the parse tree and any parse errors — to the UI.
    void showParserOutput(const TranslationUnit* tree,
                          const vector<string>& errors);
};
