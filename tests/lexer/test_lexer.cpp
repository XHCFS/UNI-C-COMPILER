#include "lexer/Lexer.h"
#include <cassert>
#include <iostream>

static int passed = 0;
static int failed = 0;

#define ASSERT_EQ(a, b, msg) \
    if ((a) == (b)) { ++passed; } \
    else { std::cerr << "FAIL: " << msg << "\n  expected: " #b "\n  got: " << (a) << "\n"; ++failed; }

void testKeywordRecognition() {
    Lexer lex("int float void return if else while for do");
    auto tokens = lex.tokenize();
    // First 9 tokens should all be KEYWORD
    for (int i = 0; i < 9; ++i)
        ASSERT_EQ((int)tokens[i].type, (int)TokenType::KEYWORD, "keyword " + tokens[i].lexeme);
}

void testIdentifier() {
    Lexer lex("myVar _count x1");
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].type, (int)TokenType::IDENTIFIER, "identifier myVar");
    ASSERT_EQ(tokens[0].lexeme, std::string("myVar"), "lexeme myVar");
}

void testIntLiteral() {
    Lexer lex("42 0 1000");
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].type, (int)TokenType::INT_LITERAL, "int literal 42");
    ASSERT_EQ(tokens[0].lexeme, std::string("42"), "lexeme 42");
}

void testFloatLiteral() {
    Lexer lex("3.14 0.5");
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].type, (int)TokenType::FLOAT_LITERAL, "float literal 3.14");
}

void testOperators() {
    Lexer lex("+ - * / % == != <= >= < > && || !");
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].type, (int)TokenType::PLUS,    "PLUS");
    ASSERT_EQ((int)tokens[1].type, (int)TokenType::MINUS,   "MINUS");
    ASSERT_EQ((int)tokens[4].type, (int)TokenType::PERCENT, "PERCENT");
    ASSERT_EQ((int)tokens[5].type, (int)TokenType::EQ,      "EQ");
    ASSERT_EQ((int)tokens[6].type, (int)TokenType::NEQ,     "NEQ");
    ASSERT_EQ((int)tokens[11].type, (int)TokenType::AND,    "AND");
    ASSERT_EQ((int)tokens[12].type, (int)TokenType::OR,     "OR");
}

void testAssignVsEqual() {
    Lexer lex("= ==");
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].type, (int)TokenType::ASSIGN, "ASSIGN");
    ASSERT_EQ((int)tokens[1].type, (int)TokenType::EQ,     "EQ");
}

void testLineComment() {
    Lexer lex("int x; // this is a comment\nint y;");
    auto tokens = lex.tokenize();
    // Should not produce a token for the comment
    ASSERT_EQ((int)tokens[0].type, (int)TokenType::KEYWORD,    "int");
    ASSERT_EQ((int)tokens[3].type, (int)TokenType::KEYWORD,    "int after comment");
}

void testBlockComment() {
    Lexer lex("int /* block */ x;");
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].type, (int)TokenType::KEYWORD,    "int before block comment");
    ASSERT_EQ((int)tokens[1].type, (int)TokenType::IDENTIFIER, "x after block comment");
}

void testLexicalError() {
    Lexer lex("int @x;");
    auto tokens = lex.tokenize();
    ASSERT_EQ(lex.errors().empty(), false, "should report lexical error for @");
}

void testSymbolTable() {
    Lexer lex("int myVar; float anotherVar;");
    lex.tokenize();
    auto sym = lex.symbolTable().lookup("myVar");
    ASSERT_EQ(sym.has_value(), true, "myVar in symbol table");
}

int main() {
    testKeywordRecognition();
    testIdentifier();
    testIntLiteral();
    testFloatLiteral();
    testOperators();
    testAssignVsEqual();
    testLineComment();
    testBlockComment();
    testLexicalError();
    testSymbolTable();

    std::cout << "\nLexer Tests: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}
