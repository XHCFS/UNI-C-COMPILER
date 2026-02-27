#pragma once
#include "Token.h"
#include <string>
#include <vector>
#include <memory>

struct ParseNode {
    std::string                           label;   // grammar rule or token lexeme
    Token*                                token;   // non-null for leaf nodes
    std::vector<std::unique_ptr<ParseNode>> children;

    explicit ParseNode(std::string label, Token* token = nullptr)
        : label(std::move(label)), token(token) {}

    void addChild(std::unique_ptr<ParseNode> child) {
        children.push_back(std::move(child));
    }
};

void printParseTree(const ParseNode* node, int depth = 0);
