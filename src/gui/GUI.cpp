#include "GUI.h"
#include <iostream>
using namespace std;

// No-op constructor; all setup will move to a real GUI init when implemented.
GUI::GUI() {}

// Prints a placeholder banner to stdout; will launch the event loop once the GUI is implemented.
void GUI::run() {
    cout << "C Compiler - Lexer & Parser\n";
    cout << "GUI not yet implemented. See GUI.h for interface.\n";
}

// Dumps tokens, symbol-table entries, and errors to stdout as a temporary substitute for real UI panels.
void GUI::showLexerOutput(const vector<Token>& tokens,
                          const SymbolTable& table,
                          const vector<string>& errors) {
    cout << "\n--- Tokens ---\n";
    for (const auto& tok : tokens) {
        cout << "[" << tokenTypeToString(tok.getType()) << "] "
                  << tok.getLexeme() << "  (line " << tok.getLine() << ")\n";
    }
    cout << "\n--- Symbol Table ---\n";
    for (const auto& [name, entry] : table.getAllEntries())
        cout << name << "  (line " << entry.getLine() << ")\n";
    if (!errors.empty()) {
        cout << "--- Lexer Errors ---\n";
        for (const auto& e : errors) cout << e << "\n";
    }
}

// Prints the parse tree and any parser errors to stdout as a temporary substitute for a real tree view.
void GUI::showParserOutput(const ParseNode* tree,
                           const vector<string>& errors) {
    cout << "\n--- Parse Tree ---\n";
    printParseTree(tree);
    if (!errors.empty()) {
        cout << "--- Parser Errors ---\n";
        for (const auto& e : errors) cout << e << "\n";
    }
}
