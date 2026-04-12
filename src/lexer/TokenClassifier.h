#pragma once
#include "Token.h"
using namespace std;

// Returns true if c can legally begin a C identifier (letter or underscore).
bool isIdentifierStart(char c);

// Returns true if c can legally appear inside a C identifier (letter, digit, or underscore).
bool isIdentifierPart (char c);

// Returns true if c is an ASCII decimal digit ('0'–'9').
bool isDigit (char c);
// Returns true if c is any whitespace character (space, tab, newline, carriage return, form-feed, vertical tab).
bool  isWhitespace (char c);

// Returns true if c can be the first character of a C operator.
bool isOperatorStart (char c);

// Returns true if c is one of the eight C delimiter characters: ( ) { } [ ] ; ,
bool isDelimiterChar (char c);

// Maps a single character to its TokenType; returns INVALID for unrecognised characters.
TokenType classifySingleChar(char c);

// Maps a two-character sequence to its TokenType (e.g. '+','=' → PLUS_ASSIGN); returns INVALID if no match.
TokenType classifyDoubleChar(char first, char second);
