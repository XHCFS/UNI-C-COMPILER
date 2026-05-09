#pragma once
#include "Token.h"


// Stateless free functions that classify TokenType values for the parser's
// dispatch logic. Plays the same structural role for tokens that
// TokenClassifier plays for characters.

// Type-specifier keyword: void/char/short/int/long/float/double/signed/unsigned/_Bool/bool.
bool isTypeSpecifier(TokenType t);

// Type-qualifier keyword: const/volatile/restrict.
bool isTypeQualifier(TokenType t);

// Storage-class keyword: auto/register/static/extern/typedef.
bool isStorageClassSpecifier(TokenType t);

// True if t starts a declaration (any type-specifier, qualifier, or storage class).
// Used by parseStatement() to disambiguate declaration vs expression statement
// inside a compound block.
bool isDeclarationStart(TokenType t);

// Assignment operator: ASSIGN through RSHIFT_ASSIGN.
bool isAssignmentOperator(TokenType t);

// Prefix-unary operator the parser handles in parseUnary():
// + - ! ~ ++ -- * &.
bool isUnaryPrefixOperator(TokenType t);

// Postfix operator the parser handles in parsePostfix(): ++ and -- only.
bool isPostfixOperator(TokenType t);

// Literal token kind: INT/FLOAT/CHAR/STRING_LITERAL.
bool isLiteralToken(TokenType t);
