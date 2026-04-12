#pragma once
#include "CharStream.h"
#include "ReservedWords.h"
#include "LexerError.h"
#include "Token.h"
#include "SymbolTable.h"
#include <string>
#include <vector>
using namespace std;

// Converts a C source string into a flat sequence of Tokens.
// Identifiers are inserted into the provided SymbolTable as they are found.
// Errors are collected rather than thrown, allowing the scan to continue after bad input.
class Lexer {
public:
    // Constructs the lexer with the source text and a reference to the shared symbol table.
    Lexer(const string& source, SymbolTable& symbolTable);

    // Scans the entire source and returns the complete list of tokens (excluding the EOF sentinel).
    vector<Token>              tokenize();

    // Returns the list of errors encountered during the most recent tokenize() call.
    const vector<LexerError>&  getErrors() const;

private:
    CharStream input_;         // Character-level reader over the source text.
    ReservedWords  reservedWords_; // Keyword lookup table used during identifier scanning.
    SymbolTable& symbolTable_;   // Shared symbol table; the lexer inserts identifiers here.
    vector<Token> tokens_;  // Accumulated token output, built by tokenize().
    vector<LexerError> errors_;  // Accumulated error list, built during scanning.

    // Reads and returns the next single token from the input stream.
    Token nextToken();

    // Advances the stream past any whitespace characters, updating line/column tracking.
    void skipWhitespace();

    // Skips a // single-line or /* block comment; returns true if a comment was consumed.
    bool skipComment();

    // Scans a run of identifier characters; returns a keyword token or IDENTIFIER token.
    Token scanIdentifierOrKeyword();

    // Scans an integer or floating-point numeric literal.
    Token scanNumber();

    // Scans a single-quoted character literal, handling escape sequences.
    Token scanCharLiteral();

    // Scans a double-quoted string literal, handling escape sequences.
    Token scanStringLiteral();

    // Scans a one-, two-, or three-character operator or delimiter token.
    Token scanOperatorOrDelimiter();

    // Constructs and returns a Token with the given fields (thin factory wrapper).
    Token makeToken(TokenType type, const string& lexeme, int line, int column);

    // Appends a LexerError to errors_ with the supplied message and source location.
    void addError(const string& message, int line, int column,
                   const string& text = "");

    // Consumes the invalid character plus any attached identifier characters as one error (e.g. "@yz" → one error, not "@" + identifier "yz").
    void recoverFromInvalidCharacter();

    // Advances past the rest of the current line to resync after an unterminated string.
    void recoverFromUnterminatedString();
};
