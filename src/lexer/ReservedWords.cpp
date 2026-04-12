#include "ReservedWords.h"
using namespace std;

// Populates keywordMap_ with every C keyword from C89 through C23.
ReservedWords::ReservedWords() {
    keywordMap_ = {
        // Control flow
        {"if",             TokenType::KW_IF},
        {"else",           TokenType::KW_ELSE},
        {"for",            TokenType::KW_FOR},
        {"while",          TokenType::KW_WHILE},
        {"do",             TokenType::KW_DO},
        {"switch",         TokenType::KW_SWITCH},
        {"case",           TokenType::KW_CASE},
        {"default",        TokenType::KW_DEFAULT},
        {"break",          TokenType::KW_BREAK},
        {"continue",       TokenType::KW_CONTINUE},
        {"return",         TokenType::KW_RETURN},
        {"goto",           TokenType::KW_GOTO},
        // Type specifiers
        {"void",           TokenType::KW_VOID},
        {"char",           TokenType::KW_CHAR},
        {"short",          TokenType::KW_SHORT},
        {"int",            TokenType::KW_INT},
        {"long",           TokenType::KW_LONG},
        {"float",          TokenType::KW_FLOAT},
        {"double",         TokenType::KW_DOUBLE},
        {"signed",         TokenType::KW_SIGNED},
        {"unsigned",       TokenType::KW_UNSIGNED},
        // Type qualifiers
        {"const",          TokenType::KW_CONST},
        {"volatile",       TokenType::KW_VOLATILE},
        {"restrict",       TokenType::KW_RESTRICT},
        // Storage class
        {"auto",           TokenType::KW_AUTO},
        {"register",       TokenType::KW_REGISTER},
        {"static",         TokenType::KW_STATIC},
        {"extern",         TokenType::KW_EXTERN},
        {"typedef",        TokenType::KW_TYPEDEF},
        // Function specifier
        {"inline",         TokenType::KW_INLINE},
        // Composite types
        {"struct",         TokenType::KW_STRUCT},
        {"union",          TokenType::KW_UNION},
        {"enum",           TokenType::KW_ENUM},
        // Expression keywords
        {"sizeof",         TokenType::KW_SIZEOF},
        // C99
        {"_Bool",          TokenType::KW__BOOL},
        {"_Complex",       TokenType::KW__COMPLEX},
        {"_Imaginary",     TokenType::KW__IMAGINARY},
        // C11
        {"_Alignas",       TokenType::KW__ALIGNAS},
        {"_Alignof",       TokenType::KW__ALIGNOF},
        {"_Atomic",        TokenType::KW__ATOMIC},
        {"_Generic",       TokenType::KW__GENERIC},
        {"_Noreturn",      TokenType::KW__NORETURN},
        {"_Static_assert", TokenType::KW__STATIC_ASSERT},
        {"_Thread_local",  TokenType::KW__THREAD_LOCAL},
        // C23
        {"alignas",        TokenType::KW_ALIGNAS},
        {"alignof",        TokenType::KW_ALIGNOF},
        {"bool",           TokenType::KW_BOOL},
        {"constexpr",      TokenType::KW_CONSTEXPR},
        {"false",          TokenType::KW_FALSE},
        {"nullptr",        TokenType::KW_NULLPTR},
        {"static_assert",  TokenType::KW_STATIC_ASSERT},
        {"thread_local",   TokenType::KW_THREAD_LOCAL},
        {"true",           TokenType::KW_TRUE},
        {"typeof",         TokenType::KW_TYPEOF},
        {"typeof_unqual",  TokenType::KW_TYPEOF_UNQUAL},
        {"_BitInt",        TokenType::KW__BITINT},
        {"_Decimal32",     TokenType::KW__DECIMAL32},
        {"_Decimal64",     TokenType::KW__DECIMAL64},
        {"_Decimal128",    TokenType::KW__DECIMAL128},
    };
}

// Performs a hash-map lookup; O(1) average case.
bool ReservedWords::isKeyword(const string& word) const {
    return keywordMap_.count(word) > 0;
}

// Returns the mapped TokenType, or INVALID if the word is not a keyword.
TokenType ReservedWords::getKeywordType(const string& word) const {
    auto it = keywordMap_.find(word);
    return it != keywordMap_.end() ? it->second : TokenType::INVALID;
}
