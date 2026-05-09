#include "TokenStream.h"
using namespace std;

// Builds the sentinel Token returned past end of stream. Carries the line/col
// of the last real token so callers can attach error messages to a sensible
// location. Falls back to 1:1 when the stream is empty.
static Token buildEofSentinel(const vector<Token>& tokens) {
    if (tokens.empty()) {
        return Token(TokenType::INVALID, "", 1, 1);
    }
    const Token& last = tokens.back();
    return Token(TokenType::INVALID, "", last.getLine(), last.getColumn());
}

// Stores the token vector reference and builds the EOF sentinel; pos_ starts at 0.
TokenStream::TokenStream(const vector<Token>& tokens)
    : tokens_(tokens), pos_(0), eofSentinel_(buildEofSentinel(tokens)) {}

// Returns tokens_[pos_+offset] when in range, otherwise the EOF sentinel.
const Token& TokenStream::peek(int offset) const {
    size_t idx = pos_ + static_cast<size_t>(offset);
    if (idx >= tokens_.size()) return eofSentinel_;
    return tokens_[idx];
}

// Returns the current token and advances pos_; returns the sentinel without
// advancing when already past end.
const Token& TokenStream::get() {
    if (pos_ >= tokens_.size()) return eofSentinel_;
    return tokens_[pos_++];
}

// True when pos_ has reached or passed the end of the token vector.
bool TokenStream::eof() const { return pos_ >= tokens_.size(); }

// Returns the 1-based line of the next-to-be-consumed token; falls back to the
// sentinel's line at EOF.
int TokenStream::getLine() const {
    return peek().getLine();
}

// Returns the 1-based column of the next-to-be-consumed token.
int TokenStream::getColumn() const {
    return peek().getColumn();
}

// Consumes the current token only when its type equals expected.
bool TokenStream::match(TokenType expected) {
    if (pos_ < tokens_.size() && tokens_[pos_].getType() == expected) {
        ++pos_;
        return true;
    }
    return false;
}

// Peek-only check: true iff the next token has type expected.
bool TokenStream::check(TokenType expected) const {
    return peek().getType() == expected;
}
