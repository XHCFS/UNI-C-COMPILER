#pragma once
#include <string>
using namespace std;

// Represents a single lexical error: a message, its source location, and
// the offending text. Errors are collected into a vector so the lexer can
// recover and continue rather than aborting on the first bad token.
class LexerError {
public:
    // Constructs an error record; offendingText is optional and defaults to empty.
    LexerError(string message, int line, int column,
               string offendingText = "");

    // Returns a formatted string: "LexerError [line:col] message ("text")".
    string toString()    const;

    // Returns the plain error message without location or offending text.
    const string& getMessage()  const;

    // Returns the 1-based line number where the error occurred.
    int                getLine()     const;

    // Returns the 1-based column number where the error occurred.
    int                getColumn()   const;

private:
    string message_;       // Human-readable description of what went wrong.
    int line_;          // Source line where the error was detected.
    int column_;        // Source column where the error was detected.
    string offendingText_; // The exact text that triggered the error, if available.
};
