#pragma once
#include <string>
using namespace  std;

// Exhaustive enumeration of every token category the lexer can produce.
// The ordering of enumerators within each group is load-bearing: isKeyword(),
// isLiteral(), isOperator(), and isDelimiter() all rely on contiguous ranges.
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

// A single lexical unit produced by the Lexer, carrying its classification,
// raw text, and source location.
class Token {
public:
    // Constructs a token with the given type, text, and source position.
    Token(TokenType type, string lexeme, int line, int column);

    // Returns the classification of this token (e.g. KW_INT, IDENTIFIER).
    TokenType getType()  const;

    // Returns the exact text slice from the source that produced this token.
    const string& getLexeme() const;

    // Returns the 1-based source line on which this token starts.
    int getLine() const;

    // Returns the 1-based source column on which this token starts.
    int getColumn() const;

    // Returns true if this token is any C keyword (KW_IF … KW__DECIMAL128).
    bool isKeyword() const;

    // Returns true if this token is an integer, float, char, or string literal.
    bool isLiteral() const;

    // Returns true if this token is any operator (arithmetic through ternary colon).
    bool isOperator()  const;

    // Returns true if this token is a delimiter: ( ) { } [ ] ; ,
    bool isDelimiter() const;

    // Returns a human-readable string in the form [TYPE 'lexeme' L<line>:C<col>].
    string toString() const;

private:
    TokenType  type_;    // Enum tag that classifies this token.
    string lexeme_;  // Raw source text of the token.
    int line_;    // 1-based line number where the token begins.
    int  column_;  // 1-based column number where the token begins.
};

// Converts a TokenType enumerator to its name as a string (e.g. KW_INT → "KW_INT").
string tokenTypeToString(TokenType type);
