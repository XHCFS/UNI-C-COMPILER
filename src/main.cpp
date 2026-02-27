#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "gui/GUI.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        GUI gui;
        gui.run();
        return 0;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open file '" << argv[1] << "'\n";
        return 1;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    std::cout << "=== Lexer Output ===\n";
    for (const auto& tok : tokens) {
        std::cout << "Line " << tok.line << "  "
                  << tokenTypeToString(tok.type)
                  << "  '" << tok.lexeme << "'\n";
    }
    lexer.symbolTable().print();

    if (!lexer.errors().empty()) {
        std::cout << "=== Lexer Errors ===\n";
        for (const auto& e : lexer.errors()) std::cout << e << "\n";
    }

    Parser parser(tokens);
    auto tree = parser.parse();

    std::cout << "=== Parse Tree ===\n";
    printParseTree(tree.get());

    if (!parser.errors().empty()) {
        std::cout << "=== Parser Errors ===\n";
        for (const auto& e : parser.errors()) std::cout << e << "\n";
    }

    return lexer.errors().empty() && parser.errors().empty() ? 0 : 1;
}
