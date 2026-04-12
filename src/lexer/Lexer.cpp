#include "Lexer.h"
#include "TokenClassifier.h"
using namespace std;

// Binds the source text to a CharStream and stores a reference to the shared symbol table.
Lexer::Lexer(const string& source, SymbolTable& symbolTable)
    : input_(source), symbolTable_(symbolTable) {}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

// Drives the scan loop until EOF, collecting every non-sentinel token into tokens_.
vector<Token> Lexer::tokenize() {
    while (!input_.eof()) {
        Token t = nextToken();
        if (!t.getLexeme().empty())   // discard EOF sentinel
            tokens_.push_back(t);
    }
    return tokens_;
}

// Returns the error list accumulated during the most recent tokenize() call.
const vector<LexerError>& Lexer::getErrors() const { return errors_; }

// ---------------------------------------------------------------------------
// Core scan loop
// ---------------------------------------------------------------------------

// Skips whitespace and comments, then dispatches to the appropriate scanner
// based on the next character; loops on invalid characters instead of recurring.
Token Lexer::nextToken() {
    while (true) {
        // Skip whitespace and comments alternately; guard EOF each cycle.
        while (true) {
            skipWhitespace();
            if (input_.eof())
                // EOF sentinel: empty lexeme + INVALID; tokenize() discards it.
                return makeToken(TokenType::INVALID, "", input_.getLine(), input_.getColumn());
            if (!skipComment()) break;
        }

        char c = input_.peek();

        if (isIdentifierStart(c)) return scanIdentifierOrKeyword();
        if (isDigit(c)) return scanNumber();
        if (c == '"') return scanStringLiteral();
        if (c == '\'') return scanCharLiteral();
        if (isOperatorStart(c) || isDelimiterChar(c)) return scanOperatorOrDelimiter();

        // Unknown character: record error, consume, and loop (no recursion).
        recoverFromInvalidCharacter();
    }
}

// ---------------------------------------------------------------------------
// Whitespace / comment skipping
// ---------------------------------------------------------------------------

// Calls input_.get() for each whitespace character, updating line/column via CharStream.
void Lexer::skipWhitespace() {
    while (!input_.eof() && isWhitespace(input_.peek()))
        input_.get();
}

// Detects // and /* comment styles, consumes them entirely, and returns true if one was found.
bool Lexer::skipComment() {
    // Single-line comment  //
    if (input_.peek(0) == '/' && input_.peek(1) == '/') {
        while (!input_.eof() && input_.peek() != '\n')
            input_.get();
        return true;
    }
    // Block comment  /* ... */
    if (input_.peek(0) == '/' && input_.peek(1) == '*') {
        int startLine = input_.getLine();
        int startCol  = input_.getColumn();
        input_.get(); // '/'
        input_.get(); // '*'
        while (true) {
            if (input_.eof()) {
                addError("unterminated block comment", startLine, startCol, "/*");
                return true;
            }
            if (input_.peek(0) == '*' && input_.peek(1) == '/') {
                input_.get(); // '*'
                input_.get(); // '/'
                return true;
            }
            input_.get();
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Token scanners
// ---------------------------------------------------------------------------

// Reads a maximal identifier string, looks it up in ReservedWords, and emits a keyword
// or IDENTIFIER token; inserts identifiers into the symbol table.
Token Lexer::scanIdentifierOrKeyword() {
    int line = input_.getLine();
    int col = input_.getColumn();
    string lexeme = input_.consumeWhile(isIdentifierPart);

    TokenType type = reservedWords_.getKeywordType(lexeme);
    if (type == TokenType::INVALID)
        type = TokenType::IDENTIFIER;

    Token tok = makeToken(type, lexeme, line, col);
    if (type == TokenType::IDENTIFIER)
        symbolTable_.insert(tok);
    return tok;
}

// Reads an integer or float literal; reports an error for invalid suffixes like "1abc".
Token Lexer::scanNumber() {
    int line = input_.getLine();
    int col = input_.getColumn();
    string lexeme = input_.consumeWhile(isDigit);

    bool isFloat = false;

    // Fractional part: require at least one digit after the dot.
    if (input_.peek(0) == '.' && isDigit(input_.peek(1))) {
        isFloat = true;
        lexeme += input_.get(); // '.'
        lexeme += input_.consumeWhile(isDigit);
    }

    // Invalid suffix: a letter or underscore glued to a number with no whitespace
    // (e.g. 1num, 3.14abc). Consume the whole run and report one error token.
    if (isIdentifierStart(input_.peek())) {
        lexeme += input_.consumeWhile(isIdentifierPart);
        addError("invalid numeric literal", line, col, lexeme);
        return makeToken(TokenType::INVALID, lexeme, line, col);
    }

    return makeToken(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL,
                     lexeme, line, col);
}

// Reads a single-quoted character literal, handling one escape sequence or plain character.
Token Lexer::scanCharLiteral() {
    int line = input_.getLine();
    int col = input_.getColumn();
    string lexeme;
    lexeme += input_.get(); // opening '

    if (input_.eof() || input_.peek() == '\n') {
        addError("unterminated character literal", line, col, lexeme);
        return makeToken(TokenType::CHAR_LITERAL, lexeme, line, col);
    }

    if (input_.peek() == '\\') {
        lexeme += input_.get(); // backslash
        if (!input_.eof())
            lexeme += input_.get(); // escaped char
    } else {
        lexeme += input_.get(); // normal char
    }

    if (!input_.eof() && input_.peek() == '\'') {
        lexeme += input_.get(); // closing '
    } else {
        addError("unterminated character literal", line, col, lexeme);
    }
    return makeToken(TokenType::CHAR_LITERAL, lexeme, line, col);
}

// Reads a double-quoted string literal, handling escape sequences until the closing quote.
Token Lexer::scanStringLiteral() {
    int line = input_.getLine();
    int col  = input_.getColumn();
    string lexeme;
    lexeme += input_.get(); // opening "

    while (true) {
        if (input_.eof() || input_.peek() == '\n') {
            addError("unterminated string literal", line, col, lexeme);
            recoverFromUnterminatedString();
            return makeToken(TokenType::STRING_LITERAL, lexeme, line, col);
        }
        if (input_.peek() == '"') {
            lexeme += input_.get(); // closing "
            break;
        }
        if (input_.peek() == '\\') {
            lexeme += input_.get(); // backslash
            if (!input_.eof())
                lexeme += input_.get(); // escaped char
        } else {
            lexeme += input_.get();
        }
    }
    return makeToken(TokenType::STRING_LITERAL, lexeme, line, col);
}

// Peeks up to three characters to identify <<= / >>= first, then two-char operators,
// then falls back to single-character classification.
Token Lexer::scanOperatorOrDelimiter() {
    int  line = input_.getLine();
    int  col  = input_.getColumn();
    char c0   = input_.peek(0);
    char c1   = input_.peek(1);
    char c2   = input_.peek(2);

    // Three-character compound shift-assignment: <<= and >>=
    if ((c0 == '<' && c1 == '<' && c2 == '=') ||
        (c0 == '>' && c1 == '>' && c2 == '=')) {
        string lex(1, input_.get());
        lex += input_.get();
        lex += input_.get();
        TokenType type = (c0 == '<') ? TokenType::LSHIFT_ASSIGN
                                     : TokenType::RSHIFT_ASSIGN;
        return makeToken(type, lex, line, col);
    }

    // Two-character operators
    TokenType two = classifyDoubleChar(c0, c1);
    if (two != TokenType::INVALID) {
        string lex(1, input_.get());
        lex += input_.get();
        return makeToken(two, lex, line, col);
    }

    // Single-character operators and delimiters
    string lex(1, input_.get());
    return makeToken(classifySingleChar(c0), lex, line, col);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Wraps the Token constructor; centralises token creation for easier future changes.
Token Lexer::makeToken(TokenType type, const string& lexeme,
                       int line, int column) {
    return Token(type, lexeme, line, column);
}

// Appends a new LexerError to errors_ with the given message and source location.
void Lexer::addError(const string& message, int line, int column,
                     const string& text) {
    errors_.emplace_back(message, line, column, text);
}

// Consumes the invalid character and any immediately following identifier characters
// as one unit, so "@yz" is reported as a single error rather than "@" + identifier "yz".
void Lexer::recoverFromInvalidCharacter() {
    int  line = input_.getLine();
    int  col  = input_.getColumn();
    string text(1, input_.get());              // consume the invalid character
    text += input_.consumeWhile(isIdentifierPart); // consume any attached letters/digits/_
    addError("invalid character sequence", line, col, text);
}

// Advances past the remainder of the current line so the next line can be scanned cleanly
// after an unterminated string literal.
void Lexer::recoverFromUnterminatedString() {
    while (!input_.eof() && input_.peek() != '\n')
        input_.get();
}

