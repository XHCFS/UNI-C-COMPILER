#pragma once
#include <string>
#include <functional>
using namespace std;

// Wraps a source string and provides character-level read access with
// automatic line/column tracking for error reporting.
class CharStream {
public:
    // Constructs the stream from the full source string; position starts at the beginning.
    explicit CharStream(const string& source);

    // Returns the character at pos+offset without consuming it; returns '\0' past end.
    char peek(int offset = 0) const;

    // Consumes and returns the current character, advancing position and updating line/column.
    char get();

    // Returns true when all characters have been consumed.
    bool eof() const;

    // Returns the current 1-based line number.
    int getLine()   const;

    // Returns the current 1-based column number.
    int getColumn() const;

    // Consumes the current character only if it equals expected; returns true on match.
    bool match(char expected);

    // Consumes and accumulates characters as long as pred returns true; returns the collected string.
    string consumeWhile(function<bool(char)> pred);

private:
    string source_;  // Immutable copy of the full source text.
    size_t pos_;     // Index of the next character to be read.
    int line_;    // Current 1-based line number, incremented on each '\n'.
    int column_;  // Current 1-based column number, reset to 1 after each '\n'.
};
