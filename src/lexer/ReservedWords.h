#pragma once
#include "Token.h"
#include <string>
#include <unordered_map>
using namespace std;

// Maintains the compile-time mapping from C keyword strings to their TokenType values.
// Used by the Lexer to distinguish keywords from user identifiers during scanning.
class ReservedWords {
public:
    // Populates keywordMap_ with all C keywords from C89 through C23.
    ReservedWords();

    // Returns true if word is a recognised C keyword.
    bool isKeyword(const string& word)  const;

    // Returns the TokenType for a keyword string, or TokenType::INVALID if not found.
    TokenType getKeywordType(const string& word) const;

private:
    // Maps each keyword string (e.g. "int") to its corresponding TokenType (e.g. KW_INT).
    unordered_map<string, TokenType> keywordMap_;
};
