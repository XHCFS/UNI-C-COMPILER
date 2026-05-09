#include "CharStream.h"
using namespace  std;
// Stores the source string and starts the cursor at line 1, column 1.
CharStream::CharStream(const string& source)
    : source_(source), pos_(0), line_(1), column_(1) {}

// Returns the character at pos_+offset without moving the cursor; '\0' if out of range.
char CharStream::peek(int offset) const {
    size_t idx = pos_ + static_cast<size_t>(offset);
    if (idx >= source_.size()) return '\0';
    return source_[idx];
}

// Advances pos_ by one, increments line_ on newline or column_ otherwise, and returns the character.
char CharStream::get() {
    if (pos_ >= source_.size()) return '\0';
    char c = source_[pos_++];
    if (c == '\n') { ++line_; column_ = 1; }
    else  { ++column_; }
    return c;
}

// True when pos_ has reached or passed the end of the source string.
bool CharStream::eof() const { return pos_ >= source_.size(); }

// Returns the 1-based line number of the character that will be returned by the next get().
int CharStream::getLine() const { return line_; }

// Returns the 1-based column number of the character that will be returned by the next get().
int CharStream::getColumn() const { return column_; }

// Consumes one character only when it equals expected; returns true on a match.
bool CharStream::match(char expected) {
    if (pos_ < source_.size() && source_[pos_] == expected) {
        get();
        return true;
    }
    return false;
}

// Repeatedly calls get() while pred returns true, accumulating the characters into a string.
string CharStream::consumeWhile(function<bool(char)> pred) {
    string result;
    while (!eof() && pred(peek())) {
        result += get();
    }
    return result;
}
