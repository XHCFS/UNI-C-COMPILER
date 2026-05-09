#include "TokenClassifier.h"
#include <cctype>
using namespace std;

// True for letters and underscore — the only characters that can open a C identifier.
bool isIdentifierStart(char c) {
    return isalpha(static_cast<unsigned char>(c)) || c == '_';
}

// True for letters, digits, and underscore — characters that may follow the first in an identifier.
bool isIdentifierPart(char c) {
    return isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// True for the ten ASCII decimal digits '0' through '9'.
bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

// True for any of the six C whitespace characters: space, tab, newline, carriage-return, form-feed, vertical-tab.
bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r'
        || c == '\f' || c == '\v';
}

// True if c can begin a C operator; used by nextToken() to dispatch to scanOperatorOrDelimiter().
bool isOperatorStart(char c) {
    return c=='+' || c=='-' || c=='*' || c=='/' || c=='%'
        || c=='=' || c=='!' || c=='<' || c=='>'
        || c=='&' || c=='|' || c=='^' || c=='~'
        || c=='.' || c=='?' || c==':';
}

// True for the eight delimiter characters that are always single-character tokens.
bool isDelimiterChar(char c) {
    return c=='(' || c==')' || c=='{' || c=='}'
        || c=='[' || c==']' || c==';' || c==',';
}

// Maps a single character to its unambiguous TokenType; returns INVALID for unknown characters.
TokenType classifySingleChar(char c) {
    switch (c) {
        case '+': return TokenType::PLUS;
        case '-': return TokenType::MINUS;
        case '*': return TokenType::STAR;
        case '/': return TokenType::SLASH;
        case '%': return TokenType::PERCENT;
        case '=': return TokenType::ASSIGN;
        case '!': return TokenType::NOT;
        case '<': return TokenType::LT;
        case '>': return TokenType::GT;
        case '&': return TokenType::BITWISE_AND;
        case '|': return TokenType::BITWISE_OR;
        case '^': return TokenType::BITWISE_XOR;
        case '~': return TokenType::BITWISE_NOT;
        case '.': return TokenType::DOT;
        case '?': return TokenType::QUESTION;
        case ':': return TokenType::COLON;
        case '(': return TokenType::LPAREN;
        case ')': return TokenType::RPAREN;
        case '{': return TokenType::LBRACE;
        case '}': return TokenType::RBRACE;
        case '[': return TokenType::LBRACKET;
        case ']': return TokenType::RBRACKET;
        case ';': return TokenType::SEMICOLON;
        case ',': return TokenType::COMMA;
        default:  return TokenType::INVALID;
    }
}

// Checks all valid two-character C operators (++, +=, ->, ==, !=, <=, <<, &&, ||, etc.);
// returns INVALID when the pair does not form a recognised two-character operator.
TokenType classifyDoubleChar(char a, char b) {
    switch (a) {
        case '+': if (b=='+') return TokenType::INCREMENT;
                  if (b=='=') return TokenType::PLUS_ASSIGN; break;
        case '-': if (b=='-') return TokenType::DECREMENT;
                  if (b=='=') return TokenType::MINUS_ASSIGN;
                  if (b=='>') return TokenType::ARROW; break;
        case '*': if (b=='=') return TokenType::STAR_ASSIGN; break;
        case '/': if (b=='=') return TokenType::SLASH_ASSIGN; break;
        case '%': if (b=='=') return TokenType::PERCENT_ASSIGN; break;
        case '=': if (b=='=') return TokenType::EQ; break;
        case '!': if (b=='=') return TokenType::NEQ; break;
        case '<': if (b=='=') return TokenType::LE;
                  if (b=='<') return TokenType::LSHIFT; break;
        case '>': if (b=='=') return TokenType::GE;
                  if (b=='>') return TokenType::RSHIFT; break;
        case '&': if (b=='&') return TokenType::LOGICAL_AND;
                  if (b=='=') return TokenType::AND_ASSIGN; break;
        case '|': if (b=='|') return TokenType::LOGICAL_OR;
                  if (b=='=') return TokenType::OR_ASSIGN; break;
        case '^': if (b=='=') return TokenType::XOR_ASSIGN; break;
    }
    return TokenType::INVALID;
}
