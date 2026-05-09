#include "lexer/Lexer.h"
#include "SymbolTable.h"
#include <cassert>
#include <iostream>
using namespace std;

static int passed = 0;
static int failed = 0;

#define ASSERT_EQ(a, b, msg) \
    if ((a) == (b)) { ++passed; } \
    else { cerr << "FAIL: " << msg << "\n  expected: " #b "\n  got: " << (a) << "\n"; ++failed; }

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
    ASSERT_EQ(tokens[0].getLexeme(), string("myVar"), "lexeme myVar");
}

void testIntLiteral() {
    SymbolTable st;
    Lexer lex("42 0 1000", st);
    auto tokens = lex.tokenize();
    ASSERT_EQ((int)tokens[0].getType(), (int)TokenType::INT_LITERAL, "int literal 42");
    ASSERT_EQ(tokens[0].getLexeme(), string("42"), "lexeme 42");
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
    // Lexer inserts at scope = -1 (the default lane).
    ASSERT_EQ(st.exists("myVar", -1),       true,  "myVar in lexer lane");
    ASSERT_EQ(st.exists("anotherVar", -1),  true,  "anotherVar in lexer lane");
    ASSERT_EQ(st.exists("myVar", 0),        false, "myVar not in parser file scope");
}

// The lexer's insert(tok) call uses scope=-1 by default; second occurrence of
// the same name in source must NOT create a second entry (lexer dedups within
// its lane).
void testLexerLaneDedup() {
    SymbolTable st;
    Lexer lex("int x; int x;", st);
    lex.tokenize();
    int lexerEntries = 0;
    for (const auto& [key, entry] : st.getAllEntries())
        if (key.scope == -1 && key.name == "x") ++lexerEntries;
    ASSERT_EQ(lexerEntries, 1, "lexer lane dedups duplicate identifier names");
}

// Two declare() calls at different scopes for the same name produce two
// distinct entries — composite key permits shadowing.
void testDeclareShadowing() {
    SymbolTable st;
    Token tok(TokenType::IDENTIFIER, "x", 1, 1);
    ASSERT_EQ(st.declare(tok, 0, "int"),       true,  "declare x at scope 0");
    ASSERT_EQ(st.declare(tok, 1, "float"),     true,  "declare x at scope 1 (shadow)");
    ASSERT_EQ(st.declare(tok, 0, "int"),       false, "redeclare x at scope 0 -> false");
    ASSERT_EQ(st.exists("x", 0),               true,  "x exists at scope 0");
    ASSERT_EQ(st.exists("x", 1),               true,  "x exists at scope 1");
    ASSERT_EQ(st.exists("x", 2),               false, "x not at scope 2");

    SymbolEntry* e0 = st.find("x", 0);
    SymbolEntry* e1 = st.find("x", 1);
    ASSERT_EQ(e0 != nullptr,                   true,  "find x at scope 0");
    ASSERT_EQ(e1 != nullptr,                   true,  "find x at scope 1");
    ASSERT_EQ(e0->getDataType(), string("int"),   "x at scope 0 is int");
    ASSERT_EQ(e1->getDataType(), string("float"), "x at scope 1 is float");
    ASSERT_EQ(e0->getScope(),    0,                    "x at scope 0 has scope_=0");
    ASSERT_EQ(e1->getScope(),    1,                    "x at scope 1 has scope_=1");
}

// lookup() walks the active scope chain innermost-to-outermost and returns the
// nearest visible entry; the scope=-1 lane is invisible to lookup.
void testLookupShadowing() {
    SymbolTable st;
    Token tokSeen(TokenType::IDENTIFIER, "x", 1, 1);
    st.insert(tokSeen);                   // lexer lane: (-1, "x")
    Token tokOuter(TokenType::IDENTIFIER, "x", 2, 1);
    st.declare(tokOuter, 0, "int");       // file scope:  ( 0, "x")
    Token tokInner(TokenType::IDENTIFIER, "x", 3, 1);
    st.declare(tokInner, 2, "float");     // nested:      ( 2, "x")

    // Inside scope 2 (with active chain {0, 2}): inner shadows.
    SymbolEntry* nearest = st.lookup("x", {0, 2});
    ASSERT_EQ(nearest != nullptr,                       true,  "lookup finds x in active chain");
    ASSERT_EQ(nearest->getScope(),                      2,     "lookup returns innermost (scope 2)");
    ASSERT_EQ(nearest->getDataType(), string("float"),    "lookup returns inner-x type");

    // After popping scope 2 (active chain {0}): outer is visible.
    SymbolEntry* outer = st.lookup("x", {0});
    ASSERT_EQ(outer != nullptr,                         true,  "lookup finds outer x");
    ASSERT_EQ(outer->getScope(),                        0,     "lookup returns scope 0");

    // lookup must not return the lexer's scope=-1 entry even when nothing else
    // matches — callers pass only real scope IDs.
    SymbolEntry* missing = st.lookup("x", {7, 8});
    ASSERT_EQ(missing == nullptr,                       true,  "lookup ignores scope=-1 lane");
}

// The lexer and parser writes share the same map; one name can produce three
// coexisting entries: (-1,"x"), (0,"x"), (1,"x").
void testThreeLaneCoexistence() {
    SymbolTable st;
    Token tok(TokenType::IDENTIFIER, "x", 1, 1);
    st.insert(tok);                       // lexer at -1
    st.declare(tok, 0, "int");            // parser at 0
    st.declare(tok, 1, "float");          // parser at 1
    int countOfX = 0;
    for (const auto& [key, entry] : st.getAllEntries())
        if (key.name == "x") ++countOfX;
    ASSERT_EQ(countOfX, 3, "three coexisting entries for x at scopes -1, 0, 1");
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
    testLexerLaneDedup();
    testDeclareShadowing();
    testLookupShadowing();
    testThreeLaneCoexistence();

    cout << "\nLexer Tests: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}
