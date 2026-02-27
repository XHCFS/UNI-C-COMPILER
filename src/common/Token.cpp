#include "Token.h"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INT_LITERAL:    return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:  return "FLOAT_LITERAL";
        case TokenType::CHAR_LITERAL:   return "CHAR_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::KEYWORD:        return "KEYWORD";
        case TokenType::PLUS:           return "PLUS";
        case TokenType::MINUS:          return "MINUS";
        case TokenType::STAR:           return "STAR";
        case TokenType::SLASH:          return "SLASH";
        case TokenType::PERCENT:        return "PERCENT";
        case TokenType::LT:             return "LT";
        case TokenType::GT:             return "GT";
        case TokenType::LE:             return "LE";
        case TokenType::GE:             return "GE";
        case TokenType::EQ:             return "EQ";
        case TokenType::NEQ:            return "NEQ";
        case TokenType::AND:            return "AND";
        case TokenType::OR:             return "OR";
        case TokenType::NOT:            return "NOT";
        case TokenType::ASSIGN:         return "ASSIGN";
        case TokenType::LPAREN:         return "LPAREN";
        case TokenType::RPAREN:         return "RPAREN";
        case TokenType::LBRACE:         return "LBRACE";
        case TokenType::RBRACE:         return "RBRACE";
        case TokenType::LBRACKET:       return "LBRACKET";
        case TokenType::RBRACKET:       return "RBRACKET";
        case TokenType::SEMICOLON:      return "SEMICOLON";
        case TokenType::COMMA:          return "COMMA";
        case TokenType::END_OF_FILE:    return "EOF";
        case TokenType::UNKNOWN:        return "UNKNOWN";
        default:                        return "?";
    }
}
