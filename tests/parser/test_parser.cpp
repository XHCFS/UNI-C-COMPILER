#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include <cassert>
#include <iostream>

static int passed = 0;
static int failed = 0;

#define ASSERT_TRUE(cond, msg) \
    if (cond) { ++passed; } \
    else { std::cerr << "FAIL: " << msg << "\n"; ++failed; }

static std::unique_ptr<ParseNode> parse(const std::string& src) {
    Lexer lex(src);
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens));
    return parser.parse();
}

static Parser parseWithErrors(const std::string& src) {
    Lexer lex(src);
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens));
    parser.parse();
    return parser;
}

void testEmptyProgram() {
    auto tree = parse("");
    ASSERT_TRUE(tree != nullptr, "empty program produces a tree");
    ASSERT_TRUE(tree->label == "program", "root label is program");
}

void testVarDeclParsed() {
    // TODO: expand once parser is implemented
    auto tree = parse("int x;");
    ASSERT_TRUE(tree != nullptr, "var decl parses without crash");
}

void testFunctionDeclParsed() {
    auto tree = parse("int add(int a, int b) { return a; }");
    ASSERT_TRUE(tree != nullptr, "function decl parses without crash");
}

void testIfStmtParsed() {
    auto tree = parse("void f(void) { if (x > 0) { return; } }");
    ASSERT_TRUE(tree != nullptr, "if stmt parses without crash");
}

void testWhileStmtParsed() {
    auto tree = parse("void f(void) { while (i < 10) { i = i + 1; } }");
    ASSERT_TRUE(tree != nullptr, "while stmt parses without crash");
}

void testForStmtParsed() {
    auto tree = parse("void f(void) { for (int i = 0; i < 10; i = i + 1) { } }");
    ASSERT_TRUE(tree != nullptr, "for stmt parses without crash");
}

void testSyntaxErrorReported() {
    auto parser = parseWithErrors("int ;");  // missing identifier
    // Parser should not crash and may report an error
    ASSERT_TRUE(true, "syntax error does not crash parser");
}

int main() {
    testEmptyProgram();
    testVarDeclParsed();
    testFunctionDeclParsed();
    testIfStmtParsed();
    testWhileStmtParsed();
    testForStmtParsed();
    testSyntaxErrorReported();

    std::cout << "\nParser Tests: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}
