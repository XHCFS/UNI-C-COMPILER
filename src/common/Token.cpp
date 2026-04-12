#include "Token.h"
#include <sstream>

// ---------------------------------------------------------------------------
// Token
// ---------------------------------------------------------------------------

// This is to know if the token is a keyword, literal, operator, or a delimiter
namespace {
    constexpr int asInt(TokenType t) { return static_cast<int>(t); }
}

Token::Token(TokenType type, std::string lexeme, int line, int column)
    : type_(type), lexeme_(std::move(lexeme)), line_(line), column_(column) {}

TokenType          Token::getType()   const { return type_; }
const std::string& Token::getLexeme() const { return lexeme_; }
int                Token::getLine()   const { return line_; }
int                Token::getColumn() const { return column_; }

bool Token::isKeyword() const {
    // Relies on KW_IF...KW__DECIMAL128 being a contiguous range in the enum (see Token.h).
    return asInt(type_) >= asInt(TokenType::KW_IF)
        && asInt(type_) <= asInt(TokenType::KW__DECIMAL128);
}

bool Token::isLiteral() const {
    // Relies on INT_LITERAL...STRING_LITERAL being a contiguous range in the enum.
    return asInt(type_) >= asInt(TokenType::INT_LITERAL)
        && asInt(type_) <= asInt(TokenType::STRING_LITERAL);
}

bool Token::isOperator() const {
    // Covers all operator variants: PLUS through COLON (contiguous range).
    return asInt(type_) >= asInt(TokenType::PLUS)
        && asInt(type_) <= asInt(TokenType::COLON);
}

bool Token::isDelimiter() const {
    // Covers LPAREN through COMMA (contiguous range).
    return asInt(type_) >= asInt(TokenType::LPAREN)
        && asInt(type_) <= asInt(TokenType::COMMA);
}

std::string Token::toString() const {
    std::ostringstream oss;
    oss << "[" << tokenTypeToString(type_)
        << " '" << lexeme_ << "'"
        << " L" << line_ << ":C" << column_ << "]";
    return oss.str();
}

// ---------------------------------------------------------------------------
// tokenTypeToString
// ---------------------------------------------------------------------------

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        // Control flow keywords
        case TokenType::KW_IF:              return "KW_IF";
        case TokenType::KW_ELSE:            return "KW_ELSE";
        case TokenType::KW_FOR:             return "KW_FOR";
        case TokenType::KW_WHILE:           return "KW_WHILE";
        case TokenType::KW_DO:              return "KW_DO";
        case TokenType::KW_SWITCH:          return "KW_SWITCH";
        case TokenType::KW_CASE:            return "KW_CASE";
        case TokenType::KW_DEFAULT:         return "KW_DEFAULT";
        case TokenType::KW_BREAK:           return "KW_BREAK";
        case TokenType::KW_CONTINUE:        return "KW_CONTINUE";
        case TokenType::KW_RETURN:          return "KW_RETURN";
        case TokenType::KW_GOTO:            return "KW_GOTO";
        // Type specifier keywords
        case TokenType::KW_VOID:            return "KW_VOID";
        case TokenType::KW_CHAR:            return "KW_CHAR";
        case TokenType::KW_SHORT:           return "KW_SHORT";
        case TokenType::KW_INT:             return "KW_INT";
        case TokenType::KW_LONG:            return "KW_LONG";
        case TokenType::KW_FLOAT:           return "KW_FLOAT";
        case TokenType::KW_DOUBLE:          return "KW_DOUBLE";
        case TokenType::KW_SIGNED:          return "KW_SIGNED";
        case TokenType::KW_UNSIGNED:        return "KW_UNSIGNED";
        // Type qualifier keywords
        case TokenType::KW_CONST:           return "KW_CONST";
        case TokenType::KW_VOLATILE:        return "KW_VOLATILE";
        case TokenType::KW_RESTRICT:        return "KW_RESTRICT";
        // Storage class keywords
        case TokenType::KW_AUTO:            return "KW_AUTO";
        case TokenType::KW_REGISTER:        return "KW_REGISTER";
        case TokenType::KW_STATIC:          return "KW_STATIC";
        case TokenType::KW_EXTERN:          return "KW_EXTERN";
        case TokenType::KW_TYPEDEF:         return "KW_TYPEDEF";
        // Function specifier keywords
        case TokenType::KW_INLINE:          return "KW_INLINE";
        // Composite type keywords
        case TokenType::KW_STRUCT:          return "KW_STRUCT";
        case TokenType::KW_UNION:           return "KW_UNION";
        case TokenType::KW_ENUM:            return "KW_ENUM";
        // Expression keywords
        case TokenType::KW_SIZEOF:          return "KW_SIZEOF";
        // C99 keywords
        case TokenType::KW__BOOL:           return "KW__BOOL";
        case TokenType::KW__COMPLEX:        return "KW__COMPLEX";
        case TokenType::KW__IMAGINARY:      return "KW__IMAGINARY";
        // C11 keywords
        case TokenType::KW__ALIGNAS:        return "KW__ALIGNAS";
        case TokenType::KW__ALIGNOF:        return "KW__ALIGNOF";
        case TokenType::KW__ATOMIC:         return "KW__ATOMIC";
        case TokenType::KW__GENERIC:        return "KW__GENERIC";
        case TokenType::KW__NORETURN:       return "KW__NORETURN";
        case TokenType::KW__STATIC_ASSERT:  return "KW__STATIC_ASSERT";
        case TokenType::KW__THREAD_LOCAL:   return "KW__THREAD_LOCAL";
        // C23 keywords
        case TokenType::KW_ALIGNAS:         return "KW_ALIGNAS";
        case TokenType::KW_ALIGNOF:         return "KW_ALIGNOF";
        case TokenType::KW_BOOL:            return "KW_BOOL";
        case TokenType::KW_CONSTEXPR:       return "KW_CONSTEXPR";
        case TokenType::KW_FALSE:           return "KW_FALSE";
        case TokenType::KW_NULLPTR:         return "KW_NULLPTR";
        case TokenType::KW_STATIC_ASSERT:   return "KW_STATIC_ASSERT";
        case TokenType::KW_THREAD_LOCAL:    return "KW_THREAD_LOCAL";
        case TokenType::KW_TRUE:            return "KW_TRUE";
        case TokenType::KW_TYPEOF:          return "KW_TYPEOF";
        case TokenType::KW_TYPEOF_UNQUAL:   return "KW_TYPEOF_UNQUAL";
        case TokenType::KW__BITINT:         return "KW__BITINT";
        case TokenType::KW__DECIMAL32:      return "KW__DECIMAL32";
        case TokenType::KW__DECIMAL64:      return "KW__DECIMAL64";
        case TokenType::KW__DECIMAL128:     return "KW__DECIMAL128";
        // Identifier
        case TokenType::IDENTIFIER:         return "IDENTIFIER";
        // Literals
        case TokenType::INT_LITERAL:        return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:      return "FLOAT_LITERAL";
        case TokenType::CHAR_LITERAL:       return "CHAR_LITERAL";
        case TokenType::STRING_LITERAL:     return "STRING_LITERAL";
        // Arithmetic operators
        case TokenType::PLUS:               return "PLUS";
        case TokenType::MINUS:              return "MINUS";
        case TokenType::STAR:               return "STAR";
        case TokenType::SLASH:              return "SLASH";
        case TokenType::PERCENT:            return "PERCENT";
        case TokenType::INCREMENT:          return "INCREMENT";
        case TokenType::DECREMENT:          return "DECREMENT";
        // Bitwise operators
        case TokenType::BITWISE_AND:        return "BITWISE_AND";
        case TokenType::BITWISE_OR:         return "BITWISE_OR";
        case TokenType::BITWISE_XOR:        return "BITWISE_XOR";
        case TokenType::BITWISE_NOT:        return "BITWISE_NOT";
        case TokenType::LSHIFT:             return "LSHIFT";
        case TokenType::RSHIFT:             return "RSHIFT";
        // Logical operators
        case TokenType::LOGICAL_AND:        return "LOGICAL_AND";
        case TokenType::LOGICAL_OR:         return "LOGICAL_OR";
        case TokenType::NOT:                return "NOT";
        // Relational operators
        case TokenType::EQ:                 return "EQ";
        case TokenType::NEQ:                return "NEQ";
        case TokenType::LT:                 return "LT";
        case TokenType::GT:                 return "GT";
        case TokenType::LE:                 return "LE";
        case TokenType::GE:                 return "GE";
        // Assignment operators
        case TokenType::ASSIGN:             return "ASSIGN";
        case TokenType::PLUS_ASSIGN:        return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN:       return "MINUS_ASSIGN";
        case TokenType::STAR_ASSIGN:        return "STAR_ASSIGN";
        case TokenType::SLASH_ASSIGN:       return "SLASH_ASSIGN";
        case TokenType::PERCENT_ASSIGN:     return "PERCENT_ASSIGN";
        case TokenType::AND_ASSIGN:         return "AND_ASSIGN";
        case TokenType::OR_ASSIGN:          return "OR_ASSIGN";
        case TokenType::XOR_ASSIGN:         return "XOR_ASSIGN";
        case TokenType::LSHIFT_ASSIGN:      return "LSHIFT_ASSIGN";
        case TokenType::RSHIFT_ASSIGN:      return "RSHIFT_ASSIGN";
        // Member access operators
        case TokenType::DOT:                return "DOT";
        case TokenType::ARROW:              return "ARROW";
        // Other operators
        case TokenType::QUESTION:           return "QUESTION";
        case TokenType::COLON:              return "COLON";
        // Delimiters
        case TokenType::LPAREN:             return "LPAREN";
        case TokenType::RPAREN:             return "RPAREN";
        case TokenType::LBRACE:             return "LBRACE";
        case TokenType::RBRACE:             return "RBRACE";
        case TokenType::LBRACKET:           return "LBRACKET";
        case TokenType::RBRACKET:           return "RBRACKET";
        case TokenType::SEMICOLON:          return "SEMICOLON";
        case TokenType::COMMA:              return "COMMA";
        // Special
        case TokenType::INVALID:            return "INVALID";
        default:                            return "UNKNOWN";
    }
}
