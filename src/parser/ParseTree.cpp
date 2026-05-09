#include "ParseTree.h"
#include <iostream>

// Recursively prints each node with two spaces of indentation per depth level.
// Leaf nodes (token != nullptr) print as [label : "lexeme"]; internal nodes print as <label>.
void printParseTree(const ParseNode* node, int depth) {
    if (!node) return;
    std::string indent(depth * 2, ' ');
    if (node->token) {
        std::cout << indent << "[" << node->label << " : \"" << node->token->getLexeme() << "\"]\n";
    } else {
        std::cout << indent << "<" << node->label << ">\n";
    }
    for (const auto& child : node->children)
        printParseTree(child.get(), depth + 1);
}
