#include "ParserClassifier.h"

bool isTypeSpecifier(TokenType t) {
    switch (t) {
        case TokenType::KW_VOID:
        case TokenType::KW_CHAR:
        case TokenType::KW_SHORT:
        case TokenType::KW_INT:
        case TokenType::KW_LONG:
        case TokenType::KW_FLOAT:
        case TokenType::KW_DOUBLE:
        case TokenType::KW_SIGNED:
        case TokenType::KW_UNSIGNED:
        case TokenType::KW__BOOL:
        case TokenType::KW_BOOL:
            return true;
        default:
            return false;
    }
}

bool isTypeQualifier(TokenType t) {
    return t == TokenType::KW_CONST
        || t == TokenType::KW_VOLATILE
        || t == TokenType::KW_RESTRICT;
}

bool isStorageClassSpecifier(TokenType t) {
    switch (t) {
        case TokenType::KW_AUTO:
        case TokenType::KW_REGISTER:
        case TokenType::KW_STATIC:
        case TokenType::KW_EXTERN:
        case TokenType::KW_TYPEDEF:
            return true;
        default:
            return false;
    }
}

bool isDeclarationStart(TokenType t) {
    return isTypeSpecifier(t) || isTypeQualifier(t) || isStorageClassSpecifier(t);
}

bool isAssignmentOperator(TokenType t) {
    switch (t) {
        case TokenType::ASSIGN:
        case TokenType::PLUS_ASSIGN:
        case TokenType::MINUS_ASSIGN:
        case TokenType::STAR_ASSIGN:
        case TokenType::SLASH_ASSIGN:
        case TokenType::PERCENT_ASSIGN:
        case TokenType::AND_ASSIGN:
        case TokenType::OR_ASSIGN:
        case TokenType::XOR_ASSIGN:
        case TokenType::LSHIFT_ASSIGN:
        case TokenType::RSHIFT_ASSIGN:
            return true;
        default:
            return false;
    }
}

bool isUnaryPrefixOperator(TokenType t) {
    switch (t) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::NOT:
        case TokenType::BITWISE_NOT:
        case TokenType::STAR:
        case TokenType::BITWISE_AND:
        case TokenType::INCREMENT:
        case TokenType::DECREMENT:
            return true;
        default:
            return false;
    }
}

bool isPostfixOperator(TokenType t) {
    return t == TokenType::INCREMENT || t == TokenType::DECREMENT;
}

bool isLiteralToken(TokenType t) {
    return t == TokenType::INT_LITERAL
        || t == TokenType::FLOAT_LITERAL
        || t == TokenType::CHAR_LITERAL
        || t == TokenType::STRING_LITERAL;
}
