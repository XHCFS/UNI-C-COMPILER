#include "lexer/Lexer.h"
#include "SymbolTable.h"
#include <cassert>
#include <iostream>

static int passed = 0;
static int failed = 0;

#define ASSERT_EQ(a, b, msg) \
    if ((a) == (b)) { ++passed; } \
    else { std::cerr << "FAIL: " << msg << "\n  expected: " #b "\n  got: " << (a) << "\n"; ++failed; }

void testKeywordRecognition() {
    SymbolTable st;
    Lexer lex("int float void return if else while for do", st);
    auto tokens = lex.tokenize();
    for (int i = 0; i < 9; ++i)
        ASSERT_EQ(tokens[i].isKeyword(), true, "keyword " + tokens[i].getLexeme());
}

void testIdentifier() {
    SymbolTable st;
    Lexer lex("myVar _count x1", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].getType(), (int)TokenType::IDENTIFIER, "identifier myVar");
    ASSERT_EQ(tokens[0].getLexeme(), std::string("myVar"), "lexeme myVar");
}

void testIntLiteral() {
    SymbolTable st;
    Lexer lex("42 0 1000", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].getType(), (int)TokenType::INT_LITERAL, "int literal 42");
    ASSERT_EQ(tokens[0].getLexeme(), std::string("42"), "lexeme 42");
}

void testFloatLiteral() {
    SymbolTable st;
    Lexer lex("3.14 0.5", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].getType(), (int)TokenType::FLOAT_LITERAL, "float literal 3.14");
}

void testOperators() {
    SymbolTable st;
    Lexer lex("+ - * / % == != <= >= < > && || !", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].getType(),  (int)TokenType::PLUS,        "PLUS");
    ASSERT_EQ((int)tokens[1].getType(),  (int)TokenType::MINUS,       "MINUS");
    ASSERT_EQ((int)tokens[4].getType(),  (int)TokenType::PERCENT,     "PERCENT");
    ASSERT_EQ((int)tokens[5].getType(),  (int)TokenType::EQ,          "EQ");
    ASSERT_EQ((int)tokens[6].getType(),  (int)TokenType::NEQ,         "NEQ");
    ASSERT_EQ((int)tokens[11].getType(), (int)TokenType::LOGICAL_AND, "LOGICAL_AND");
    ASSERT_EQ((int)tokens[12].getType(), (int)TokenType::LOGICAL_OR,  "LOGICAL_OR");
}

void testAssignVsEqual() {
    SymbolTable st;
    Lexer lex("= ==", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].getType(), (int)TokenType::ASSIGN, "ASSIGN");
    ASSERT_EQ((int)tokens[1].getType(), (int)TokenType::EQ,     "EQ");
}

void testLineComment() {
    SymbolTable st;
    Lexer lex("int x; // this is a comment\nint y;", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ(tokens[0].isKeyword(), true, "int");
    ASSERT_EQ(tokens[3].isKeyword(), true, "int after comment");
}

void testBlockComment() {
    SymbolTable st;
    Lexer lex("int /* block */ x;", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ(tokens[0].isKeyword(),    true,                            "int before block comment");
    ASSERT_EQ((int)tokens[1].getType(), (int)TokenType::IDENTIFIER, "x after block comment");
}

void testLexicalError() {
    SymbolTable st;
    Lexer lex("int @x;", st);
    lex.tokenize();
    ASSERT_EQ(lex.getErrors().empty(), false, "should report lexical error for @");
}

void testSymbolTable() {
    SymbolTable st;
    Lexer lex("int myVar; float anotherVar;", st);
    lex.tokenize();
    ASSERT_EQ(st.exists("myVar"), true, "myVar in symbol table");
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
