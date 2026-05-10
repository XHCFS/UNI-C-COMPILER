#pragma once
#include "Token.h"
#include <vector>
using namespace std;


// Wraps a vector<Token> produced by the Lexer and provides token-level read
// access with 1-token lookahead. Plays the same role for the parser that
// CharStream plays for the lexer: a thin cursor with peek-ahead, consume, and
// EOF detection.
class TokenStream {
public:
    // Constructs the stream from the lexer's output. The vector is borrowed
    // by reference (not copied); the caller must keep it alive.
    explicit TokenStream(const vector<Token>& tokens);

    // Returns the token at pos + offset without consuming. Returns the EOF
    // sentinel (TokenType::INVALID, empty lexeme, line/col copied from the
    // last real token, or 1:1 if the stream is empty) when out of range.
    const Token& peek(int offset = 0) const;

    // Consumes the current token and returns it. Returns the EOF sentinel
    // past end without advancing.
    const Token& get();

    // True when pos_ has reached or passed the end of the token vector.
    bool eof() const;

    // Returns the 1-based line of the next-to-be-consumed token (or the
    // sentinel's line at EOF).
    int getLine() const;

    // Returns the 1-based column of the next-to-be-consumed token.
    int getColumn() const;

    // Consumes the current token and returns true only if its type equals
    // expected; otherwise leaves the cursor unchanged and returns false.
    bool match(TokenType expected);

    // True iff peek().getType() == expected. Does not consume.
    bool check(TokenType expected) const;

    // Returns the current cursor index. Used by callers (e.g. Parser::parse)
    // to detect whether a recursive descent step actually consumed input —
    // critical for forward-progress guarantees on garbage input.
    size_t getPos() const;

private:
    const vector<Token>& tokens_;       // non-owning reference to the lexer's output
    size_t pos_;          // index of the next token to be read
    Token  eofSentinel_;  // returned past end (TokenType::INVALID)
};
