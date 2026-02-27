#include "ParseTree.h"
#include <iostream>

void printParseTree(const ParseNode* node, int depth) {
    if (!node) return;
    std::string indent(depth * 2, ' ');
    if (node->token) {
        std::cout << indent << "[" << node->label << " : \"" << node->token->lexeme << "\"]\n";
    } else {
        std::cout << indent << "<" << node->label << ">\n";
    }
    for (const auto& child : node->children)
        printParseTree(child.get(), depth + 1);
}
