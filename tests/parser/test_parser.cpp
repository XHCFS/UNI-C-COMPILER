#include "parser/TokenStream.h"
#include "Token.h"
#include <iostream>
#include <vector>

static int passed = 0;
static int failed = 0;

#define ASSERT_EQ(a, b, msg) \
    if ((a) == (b)) { ++passed; } \
    else { std::cerr << "FAIL: " << msg << "\n  expected: " #b "\n  got: " << (a) << "\n"; ++failed; }

// Sample token sequence representing `int x = 42;` for stream-shape tests.
static std::vector<Token> sampleTokens() {
    return {
        Token(TokenType::KW_INT,      "int", 1, 1),
        Token(TokenType::IDENTIFIER,  "x",   1, 5),
        Token(TokenType::ASSIGN,      "=",   1, 7),
        Token(TokenType::INT_LITERAL, "42",  1, 9),
        Token(TokenType::SEMICOLON,   ";",   1, 11),
    };
}

void testPeekDoesNotConsume() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "peek returns first token");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "peek is idempotent");
    ASSERT_EQ(s.eof(),                  false,                 "peek does not advance to EOF");
}

void testPeekWithOffset() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ((int)s.peek(0).getType(), (int)TokenType::KW_INT,     "peek(0) -> KW_INT");
    ASSERT_EQ((int)s.peek(1).getType(), (int)TokenType::IDENTIFIER, "peek(1) -> IDENTIFIER");
    ASSERT_EQ(s.peek(2).getLexeme(),    std::string("="),           "peek(2) lexeme '='");
    // Past-end peek returns the sentinel (INVALID).
    ASSERT_EQ((int)s.peek(99).getType(), (int)TokenType::INVALID,   "peek past end -> sentinel");
}

void testGetAdvances() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    Token t0 = s.get();
    ASSERT_EQ((int)t0.getType(), (int)TokenType::KW_INT,     "get returns first token");
    Token t1 = s.get();
    ASSERT_EQ((int)t1.getType(), (int)TokenType::IDENTIFIER, "get advances to second");
    ASSERT_EQ(s.eof(),           false,                      "still not EOF after 2 gets");
}

void testEofAfterAllConsumed() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    for (size_t i = 0; i < toks.size(); ++i) (void)s.get();
    ASSERT_EQ(s.eof(), true, "EOF after consuming all 5 tokens");
    // get() past end keeps returning the sentinel without advancing further.
    Token sentinel = s.get();
    ASSERT_EQ((int)sentinel.getType(), (int)TokenType::INVALID, "get past end -> sentinel type");
    ASSERT_EQ(sentinel.getLexeme(),    std::string(""),         "sentinel lexeme empty");
    ASSERT_EQ(s.eof(),                 true,                    "eof stays true past end");
}

void testEofSentinelLineColFromLastToken() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    while (!s.eof()) (void)s.get();
    // Sentinel inherits line/col from the last real token (';' at L1:C11).
    Token sentinel = s.peek();
    ASSERT_EQ(sentinel.getLine(),   1,  "sentinel line copied from last token");
    ASSERT_EQ(sentinel.getColumn(), 11, "sentinel column copied from last token");
}

void testEofSentinelEmptyStream() {
    std::vector<Token> empty;
    TokenStream s(empty);
    ASSERT_EQ(s.eof(), true, "empty stream is EOF immediately");
    Token sentinel = s.peek();
    ASSERT_EQ((int)sentinel.getType(), (int)TokenType::INVALID, "empty stream sentinel type");
    ASSERT_EQ(sentinel.getLine(),   1, "empty stream sentinel line falls back to 1");
    ASSERT_EQ(sentinel.getColumn(), 1, "empty stream sentinel column falls back to 1");
}

void testGetLineColumnTracking() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ(s.getLine(),   1, "getLine of next token");
    ASSERT_EQ(s.getColumn(), 1, "getColumn of next token");
    s.get();
    ASSERT_EQ(s.getColumn(), 5, "getColumn moves to second token after one get");
    // At EOF, getLine/getColumn fall through to the sentinel.
    while (!s.eof()) (void)s.get();
    ASSERT_EQ(s.getLine(),   1,  "getLine at EOF -> sentinel line");
    ASSERT_EQ(s.getColumn(), 11, "getColumn at EOF -> sentinel column");
}

void testMatchConsumesOnHit() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    bool ok = s.match(TokenType::KW_INT);
    ASSERT_EQ(ok, true, "match KW_INT returns true");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::IDENTIFIER, "cursor advanced after match");
}

void testMatchLeavesCursorOnMiss() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    bool ok = s.match(TokenType::IDENTIFIER);
    ASSERT_EQ(ok, false, "match IDENTIFIER fails (head is KW_INT)");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "cursor unchanged after mismatch");
}

void testMatchAtEofIsFalse() {
    std::vector<Token> empty;
    TokenStream s(empty);
    ASSERT_EQ(s.match(TokenType::SEMICOLON), false, "match at EOF returns false");
    ASSERT_EQ(s.eof(),                       true,  "eof unchanged after match at EOF");
}

void testCheckIsPeekOnly() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ(s.check(TokenType::KW_INT),     true,  "check KW_INT true at head");
    ASSERT_EQ(s.check(TokenType::IDENTIFIER), false, "check IDENTIFIER false at head");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "check did not advance");
}

int main() {
    testPeekDoesNotConsume();
    testPeekWithOffset();
    testGetAdvances();
    testEofAfterAllConsumed();
    testEofSentinelLineColFromLastToken();
    testEofSentinelEmptyStream();
    testGetLineColumnTracking();
    testMatchConsumesOnHit();
    testMatchLeavesCursorOnMiss();
    testMatchAtEofIsFalse();
    testCheckIsPeekOnly();

    std::cout << "\nParser Tests: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}
