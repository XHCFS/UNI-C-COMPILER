#pragma once
#include <string>

enum class TokenType {

    // --- C23 Keywords ---

    // Control flow
    KW_IF, KW_ELSE, KW_FOR, KW_WHILE, KW_DO,
    KW_SWITCH, KW_CASE, KW_DEFAULT,
    KW_BREAK, KW_CONTINUE, KW_RETURN, KW_GOTO,

    // Type specifiers
    KW_VOID, KW_CHAR, KW_SHORT, KW_INT, KW_LONG,
    KW_FLOAT, KW_DOUBLE, KW_SIGNED, KW_UNSIGNED,

    // Type qualifiers
    KW_CONST, KW_VOLATILE, KW_RESTRICT,

    // Storage class specifiers
    KW_AUTO, KW_REGISTER, KW_STATIC, KW_EXTERN, KW_TYPEDEF,

    // Function specifiers
    KW_INLINE,

    // Composite types
    KW_STRUCT, KW_UNION, KW_ENUM,

    // Expression keywords
    KW_SIZEOF,

    // C99 keywords
    KW__BOOL, KW__COMPLEX, KW__IMAGINARY,

    // C11 keywords
    KW__ALIGNAS, KW__ALIGNOF, KW__ATOMIC, KW__GENERIC,
    KW__NORETURN, KW__STATIC_ASSERT, KW__THREAD_LOCAL,

    // C23 keywords
    KW_ALIGNAS, KW_ALIGNOF, KW_BOOL, KW_CONSTEXPR,
    KW_FALSE, KW_NULLPTR, KW_STATIC_ASSERT, KW_THREAD_LOCAL,
    KW_TRUE, KW_TYPEOF, KW_TYPEOF_UNQUAL,
    KW__BITINT, KW__DECIMAL32, KW__DECIMAL64, KW__DECIMAL128,

    // --- Identifier ---
    IDENTIFIER,

    // --- Literals ---
    INT_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,

    // --- Arithmetic operators ---
    PLUS,               // +
    MINUS,              // -
    STAR,               // *
    SLASH,              // /
    PERCENT,            // %
    INCREMENT,          // ++
    DECREMENT,          // --

    // --- Bitwise operators ---
    BITWISE_AND,        // &
    BITWISE_OR,         // |
    BITWISE_XOR,        // ^
    BITWISE_NOT,        // ~
    LSHIFT,             // <<
    RSHIFT,             // >>

    // --- Logical operators ---
    LOGICAL_AND,        // &&
    LOGICAL_OR,         // ||
    NOT,                // !

    // --- Relational operators ---
    EQ,                 // ==
    NEQ,                // !=
    LT,                 // <
    GT,                 // >
    LE,                 // <=
    GE,                 // >=

    // --- Assignment operators ---
    ASSIGN,             // =
    PLUS_ASSIGN,        // +=
    MINUS_ASSIGN,       // -=
    STAR_ASSIGN,        // *=
    SLASH_ASSIGN,       // /=
    PERCENT_ASSIGN,     // %=
    AND_ASSIGN,         // &=
    OR_ASSIGN,          // |=
    XOR_ASSIGN,         // ^=
    LSHIFT_ASSIGN,      // <<=
    RSHIFT_ASSIGN,      // >>=

    // --- Member access operators ---
    DOT,                // .
    ARROW,              // ->

    // --- Other operators ---
    QUESTION,           // ?
    COLON,              // :

    // --- Delimiters ---
    LPAREN,             // (
    RPAREN,             // )
    LBRACE,             // {
    RBRACE,             // }
    LBRACKET,           // [
    RBRACKET,           // ]
    SEMICOLON,          // ;
    COMMA,              // ,

    // --- Special ---
    INVALID
};

class Token {
public:
    Token(TokenType type, std::string lexeme, int line, int column);

    TokenType          getType()   const;
    const std::string& getLexeme() const;
    int                getLine()   const;
    int                getColumn() const;

    // Category checks: each tests a contiguous range in the enum.
    bool isKeyword()   const;   // any KW_* variant
    bool isLiteral()   const;   // INT/FLOAT/CHAR/STRING literal
    bool isOperator()  const;   // arithmetic, bitwise, logical, relational, assignment, member-access, ? :
    bool isDelimiter() const;   // ( ) { } [ ] ; ,

    std::string toString() const;

private:
    TokenType   type_;
    std::string lexeme_;
    int         line_;
    int         column_;
};

std::string tokenTypeToString(TokenType type);
