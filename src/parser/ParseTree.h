#pragma once
#include "Token.h"
#include <string>
#include <vector>
#include <memory>
using namespace std;

// A single node in the parse tree.
// Internal nodes represent grammar rules (label = rule name, token = nullptr).
// Leaf nodes represent matched tokens (label = token type name, token != nullptr).
struct ParseNode {
    string                           label;    // Grammar rule name or token-type name for this node.
    Token*                                token;    // Non-null only for leaf nodes; points into the Parser's token vector.
    vector<unique_ptr<ParseNode>> children; // Ordered list of child nodes owned by this node.

    // Constructs a node with the given label; token defaults to nullptr for internal nodes.
    explicit ParseNode(string label, Token* token = nullptr)
        : label(move(label)), token(token) {}

    // Appends child as the last child of this node, transferring ownership.
    void addChild(unique_ptr<ParseNode> child) {
        children.push_back(move(child));
    }
};

// Prints the parse tree rooted at node to stdout with indentation proportional to depth.
void printParseTree(const ParseNode* node, int depth = 0);
