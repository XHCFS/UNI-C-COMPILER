#pragma once
#include <string>

enum class TokenType {
    // Literals
    INT_LITERAL, FLOAT_LITERAL, CHAR_LITERAL, STRING_LITERAL,
    // Identifier / keyword
    IDENTIFIER, KEYWORD,
    // Arithmetic
    PLUS, MINUS, STAR, SLASH, PERCENT,
    // Relational
    LT, GT, LE, GE, EQ, NEQ,
    // Logical
    AND, OR, NOT,
    // Assignment
    ASSIGN,
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SEMICOLON, COMMA,
    // Special
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;
    int         column;

    Token(TokenType type, std::string lexeme, int line, int column)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column) {}
};

std::string tokenTypeToString(TokenType type);
