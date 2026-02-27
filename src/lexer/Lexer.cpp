#include "Lexer.h"
#include <cctype>
#include <unordered_set>
#include <stdexcept>
#include <sstream>

static const std::unordered_set<std::string> KEYWORDS = {
    "int", "float", "double", "char", "void",
    "return", "if", "else", "while", "for", "do",
    "break", "continue", "struct", "typedef",
    "sizeof", "const", "unsigned", "long", "short"
};

Lexer::Lexer(const std::string& source) : source_(source) {
    symbolTable_.enterScope(); // global scope
}

char Lexer::current() const {
    return pos_ < source_.size() ? source_[pos_] : '\0';
}

char Lexer::peek(int offset) const {
    size_t idx = pos_ + offset;
    return idx < source_.size() ? source_[idx] : '\0';
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') { ++line_; column_ = 1; }
    else           { ++column_; }
    return c;
}

void Lexer::skipWhitespace() {
    while (pos_ < source_.size() && std::isspace(current())) advance();
}

void Lexer::skipLineComment() {
    while (pos_ < source_.size() && current() != '\n') advance();
}

void Lexer::skipBlockComment() {
    int startLine = line_;
    advance(); advance(); // consume /*
    while (pos_ < source_.size()) {
        if (current() == '*' && peek() == '/') {
            advance(); advance();
            return;
        }
        advance();
    }
    errors_.push_back("Unterminated block comment starting at line " + std::to_string(startLine));
}

bool Lexer::isKeyword(const std::string& s) const {
    return KEYWORDS.count(s) > 0;
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line_, startCol = column_;
    std::string lexeme;
    while (pos_ < source_.size() && (std::isalnum(current()) || current() == '_'))
        lexeme += advance();

    if (isKeyword(lexeme)) return Token(TokenType::KEYWORD, lexeme, startLine, startCol);

    // Insert into symbol table if new
    if (!symbolTable_.lookup(lexeme)) {
        Symbol sym;
        sym.name       = lexeme;
        sym.dataType   = "unknown";
        sym.kind       = SymbolKind::VARIABLE;
        sym.scopeLevel = symbolTable_.currentScope();
        sym.lineNumber = startLine;
        symbolTable_.insert(sym);
    }
    return Token(TokenType::IDENTIFIER, lexeme, startLine, startCol);
}

Token Lexer::readNumber() {
    int startLine = line_, startCol = column_;
    std::string lexeme;
    bool isFloat = false;
    while (pos_ < source_.size() && std::isdigit(current())) lexeme += advance();
    if (current() == '.' && std::isdigit(peek())) {
        isFloat = true;
        lexeme += advance(); // '.'
        while (pos_ < source_.size() && std::isdigit(current())) lexeme += advance();
    }
    return Token(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL,
                 lexeme, startLine, startCol);
}

Token Lexer::readCharLiteral() {
    int startLine = line_, startCol = column_;
    std::string lexeme;
    lexeme += advance(); // opening '
    if (current() == '\\') { lexeme += advance(); lexeme += advance(); }
    else if (current() != '\'') lexeme += advance();
    if (current() == '\'') lexeme += advance();
    else errors_.push_back("Unterminated char literal at line " + std::to_string(startLine));
    return Token(TokenType::CHAR_LITERAL, lexeme, startLine, startCol);
}

Token Lexer::readStringLiteral() {
    int startLine = line_, startCol = column_;
    std::string lexeme;
    lexeme += advance(); // opening "
    while (pos_ < source_.size() && current() != '"' && current() != '\n') {
        if (current() == '\\') lexeme += advance();
        lexeme += advance();
    }
    if (current() == '"') lexeme += advance();
    else errors_.push_back("Unterminated string literal at line " + std::to_string(startLine));
    return Token(TokenType::STRING_LITERAL, lexeme, startLine, startCol);
}

Token Lexer::readOperatorOrPunct() {
    int startLine = line_, startCol = column_;
    char c = advance();
    char n = current();
    switch (c) {
        case '+': return Token(TokenType::PLUS,      "+", startLine, startCol);
        case '-': return Token(TokenType::MINUS,     "-", startLine, startCol);
        case '*': return Token(TokenType::STAR,      "*", startLine, startCol);
        case '/': return Token(TokenType::SLASH,     "/", startLine, startCol);
        case '%': return Token(TokenType::PERCENT,   "%", startLine, startCol);
        case '(': return Token(TokenType::LPAREN,    "(", startLine, startCol);
        case ')': return Token(TokenType::RPAREN,    ")", startLine, startCol);
        case '{': return Token(TokenType::LBRACE,    "{", startLine, startCol);
        case '}': return Token(TokenType::RBRACE,    "}", startLine, startCol);
        case '[': return Token(TokenType::LBRACKET,  "[", startLine, startCol);
        case ']': return Token(TokenType::RBRACKET,  "]", startLine, startCol);
        case ';': return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        case ',': return Token(TokenType::COMMA,     ",", startLine, startCol);
        case '!': if (n=='='){advance();return Token(TokenType::NEQ,"!=",startLine,startCol);}
                  return Token(TokenType::NOT, "!", startLine, startCol);
        case '<': if (n=='='){advance();return Token(TokenType::LE,"<=",startLine,startCol);}
                  return Token(TokenType::LT, "<", startLine, startCol);
        case '>': if (n=='='){advance();return Token(TokenType::GE,">=",startLine,startCol);}
                  return Token(TokenType::GT, ">", startLine, startCol);
        case '=': if (n=='='){advance();return Token(TokenType::EQ,"==",startLine,startCol);}
                  return Token(TokenType::ASSIGN, "=", startLine, startCol);
        case '&': if (n=='&'){advance();return Token(TokenType::AND,"&&",startLine,startCol);}
                  break;
        case '|': if (n=='|'){advance();return Token(TokenType::OR,"||",startLine,startCol);}
                  break;
        default: break;
    }
    std::string msg = "Unknown character '";
    msg += c;
    msg += "' at line " + std::to_string(startLine) + ":" + std::to_string(startCol);
    errors_.push_back(msg);
    return Token(TokenType::UNKNOWN, std::string(1, c), startLine, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos_ < source_.size()) {
        skipWhitespace();
        if (pos_ >= source_.size()) break;

        char c = current();

        if (c == '/' && peek() == '/') { skipLineComment();  continue; }
        if (c == '/' && peek() == '*') { skipBlockComment(); continue; }

        if (std::isalpha(c) || c == '_') { tokens.push_back(readIdentifierOrKeyword()); continue; }
        if (std::isdigit(c))             { tokens.push_back(readNumber());               continue; }
        if (c == '\'')                   { tokens.push_back(readCharLiteral());          continue; }
        if (c == '"')                    { tokens.push_back(readStringLiteral());         continue; }

        tokens.push_back(readOperatorOrPunct());
    }
    tokens.emplace_back(TokenType::END_OF_FILE, "EOF", line_, column_);
    return tokens;
}
