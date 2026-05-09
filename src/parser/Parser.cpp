#include "Parser.h"
#include "ParserClassifier.h"
using namespace std;

// Binds the token vector to a TokenStream, stores the symbol-table reference,
// and seeds scope tracking with file scope (id 0).
Parser::Parser(const vector<Token>& tokens, SymbolTable& symbolTable)
    : input_(tokens), symbolTable_(symbolTable),
      nextScopeId_(0), scopeStack_{0} {}

// Returns the accumulated parse-error list.
const vector<ParseError>& Parser::getErrors() const { return errors_; }

// Appends a new ParseError to errors_; mirrors Lexer::addError.
void Parser::addError(const string& message, int line, int column,
                      const string& text) {
    errors_.emplace_back(message, line, column, text);
}

// Match-and-consume with diagnostic on miss. On miss, reports an error using
// the CURRENT (unchanged) cursor's line/column/lexeme so the message points at
// the token the parser actually saw.
bool Parser::expect(TokenType type, const string& message) {
    if (input_.match(type)) return true;
    const Token& bad = input_.peek();
    addError(message, bad.getLine(), bad.getColumn(), bad.getLexeme());
    return false;
}

// Consume tokens until the cursor sits at a synchronisation point. SEMICOLON
// is itself consumed (we want to resume past the broken statement). Brace and
// statement-leading keywords are left in place so the next grammar method can
// decide how to handle them.
void Parser::synchronize() {
    while (!input_.eof()) {
        TokenType t = input_.peek().getType();
        switch (t) {
            case TokenType::SEMICOLON:
                input_.get();   // consume and stop
                return;
            case TokenType::LBRACE:
            case TokenType::RBRACE:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_DO:
            case TokenType::KW_RETURN:
            case TokenType::KW_BREAK:
            case TokenType::KW_CONTINUE:
                return;         // leave for the caller
            default:
                input_.get();   // skip and keep looking
                break;
        }
    }
}

// ===========================================================================
// Expression cascade
// ===========================================================================
//
// Each method snapshots line/col at entry and uses that as the position for
// any AST node it builds (so `a + b` reports `a`'s position, not `+`'s).
// Errors are recorded and propagated as nullptr operands; downstream nodes
// tolerate null children. Operators are stored as the consumed TokenType
// directly — no parallel "operator kind" enum.

// Top of the cascade. Delegates to assignment (the lowest precedence operator).
unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

// Right-associative: lhs op (parseAssignment) on hit; otherwise return the
// lhs as-is. The lhs is parsed at the next-higher level (parseConditional)
// so we don't accidentally chain ?: inside the assignment.
unique_ptr<Expr> Parser::parseAssignment() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseConditional();
    if (!isAssignmentOperator(input_.peek().getType())) return left;
    TokenType op = input_.get().getType();
    auto right = parseAssignment();   // right-associative: a = b = c -> a = (b = c)
    return make_unique<AssignExpr>(line, col, op, move(left), move(right));
}

// Right-associative ternary cond ? then : else. The 'then' branch is a full
// expression (parseExpression); the 'else' branch recurses into parseConditional
// to chain right-associative ?: cleanly.
unique_ptr<Expr> Parser::parseConditional() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto cond = parseLogicalOr();
    if (!input_.match(TokenType::QUESTION)) return cond;
    auto thenExpr = parseExpression();
    expect(TokenType::COLON, "expected ':' in conditional expression");
    auto elseExpr = parseConditional();
    return make_unique<ConditionalExpr>(line, col, move(cond), move(thenExpr), move(elseExpr));
}

// All ten left-associative binary levels share the same shape: parse the
// next-higher level, then loop while the current level's operator(s) appear,
// consuming the operator and the next-higher-level operand each iteration.
//
// We don't extract this into a template because expressing "the operator set
// for level N" cleanly in C++ is more code than just writing the 12 lines
// per level.

unique_ptr<Expr> Parser::parseLogicalOr() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseLogicalAnd();
    while (input_.peek().getType() == TokenType::LOGICAL_OR) {
        TokenType op = input_.get().getType();
        auto right = parseLogicalAnd();
        left = make_unique<BinaryExpr>(line, col, op, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseLogicalAnd() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseBitwiseOr();
    while (input_.peek().getType() == TokenType::LOGICAL_AND) {
        TokenType op = input_.get().getType();
        auto right = parseBitwiseOr();
        left = make_unique<BinaryExpr>(line, col, op, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseBitwiseOr() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseBitwiseXor();
    while (input_.peek().getType() == TokenType::BITWISE_OR) {
        TokenType op = input_.get().getType();
        auto right = parseBitwiseXor();
        left = make_unique<BinaryExpr>(line, col, op, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseBitwiseXor() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseBitwiseAnd();
    while (input_.peek().getType() == TokenType::BITWISE_XOR) {
        TokenType op = input_.get().getType();
        auto right = parseBitwiseAnd();
        left = make_unique<BinaryExpr>(line, col, op, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseBitwiseAnd() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseEquality();
    while (input_.peek().getType() == TokenType::BITWISE_AND) {
        TokenType op = input_.get().getType();
        auto right = parseEquality();
        left = make_unique<BinaryExpr>(line, col, op, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseEquality() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseRelational();
    while (true) {
        TokenType t = input_.peek().getType();
        if (t != TokenType::EQ && t != TokenType::NEQ) break;
        input_.get();
        auto right = parseRelational();
        left = make_unique<BinaryExpr>(line, col, t, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseRelational() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseShift();
    while (true) {
        TokenType t = input_.peek().getType();
        if (t != TokenType::LT && t != TokenType::GT
            && t != TokenType::LE && t != TokenType::GE) break;
        input_.get();
        auto right = parseShift();
        left = make_unique<BinaryExpr>(line, col, t, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseShift() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseAdditive();
    while (true) {
        TokenType t = input_.peek().getType();
        if (t != TokenType::LSHIFT && t != TokenType::RSHIFT) break;
        input_.get();
        auto right = parseAdditive();
        left = make_unique<BinaryExpr>(line, col, t, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseAdditive() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseMultiplicative();
    while (true) {
        TokenType t = input_.peek().getType();
        if (t != TokenType::PLUS && t != TokenType::MINUS) break;
        input_.get();
        auto right = parseMultiplicative();
        left = make_unique<BinaryExpr>(line, col, t, move(left), move(right));
    }
    return left;
}

unique_ptr<Expr> Parser::parseMultiplicative() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto left = parseUnary();
    while (true) {
        TokenType t = input_.peek().getType();
        if (t != TokenType::STAR && t != TokenType::SLASH
            && t != TokenType::PERCENT) break;
        input_.get();
        auto right = parseUnary();
        left = make_unique<BinaryExpr>(line, col, t, move(left), move(right));
    }
    return left;
}

// Prefix unary operators recurse into parseUnary so chains like `--*p` parse
// as `--(*p)`. On miss, delegate to parsePostfix.
unique_ptr<Expr> Parser::parseUnary() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    if (isUnaryPrefixOperator(input_.peek().getType())) {
        TokenType op = input_.get().getType();
        auto operand = parseUnary();
        return make_unique<UnaryExpr>(line, col, /*isPostfix=*/false, op, move(operand));
    }
    return parsePostfix();
}

// Postfix chain: ++ / -- (postfix), function call, and array indexing all
// bind tighter than prefix unary. Loops so chains like `f(a)[b]++` build up
// left-to-right: ((f(a))[b])++.
unique_ptr<Expr> Parser::parsePostfix() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto e = parsePrimary();
    while (true) {
        TokenType t = input_.peek().getType();
        if (t == TokenType::INCREMENT || t == TokenType::DECREMENT) {
            input_.get();
            e = make_unique<UnaryExpr>(line, col, /*isPostfix=*/true, t, move(e));
        } else if (t == TokenType::LPAREN) {
            input_.get();   // consume '('
            vector<unique_ptr<Expr>> args;
            if (input_.peek().getType() != TokenType::RPAREN) {
                args.push_back(parseExpression());
                while (input_.match(TokenType::COMMA))
                    args.push_back(parseExpression());
            }
            expect(TokenType::RPAREN, "expected ')' to close argument list");
            e = make_unique<CallExpr>(line, col, move(e), move(args));
        } else if (t == TokenType::LBRACKET) {
            input_.get();   // consume '['
            auto idx = parseExpression();
            expect(TokenType::RBRACKET, "expected ']' to close subscript");
            e = make_unique<IndexExpr>(line, col, move(e), move(idx));
        } else {
            break;
        }
    }
    return e;
}

// Atom dispatch. Parenthesised sub-expressions are returned as-is — grouping
// is preserved structurally by precedence, no dedicated ParenExpr node.
unique_ptr<Expr> Parser::parsePrimary() {
    const Token& tok = input_.peek();
    int line = tok.getLine();
    int col  = tok.getColumn();
    TokenType t = tok.getType();

    if (isLiteralToken(t)) {
        string lex = tok.getLexeme();
        input_.get();
        return make_unique<LiteralExpr>(line, col, t, move(lex));
    }
    if (t == TokenType::IDENTIFIER) {
        string name = tok.getLexeme();
        input_.get();
        return make_unique<IdentifierExpr>(line, col, move(name));
    }
    if (t == TokenType::LPAREN) {
        input_.get();   // consume '('
        auto inner = parseExpression();
        expect(TokenType::RPAREN, "expected ')' to close parenthesised expression");
        return inner;   // structural grouping; no ParenExpr wrapper
    }

    // Anything else cannot start an expression. Record an error, skip one
    // token to make forward progress, and return null.
    addError("expected expression", line, col, tok.getLexeme());
    if (!input_.eof()) input_.get();
    return nullptr;
}

// ===========================================================================
// Statement parsers
// ===========================================================================

// Top-level statement dispatch. Looks at the first token only; on a stray ';'
// consumes it and returns nullptr (no NullStmt node). Anything that isn't a
// statement keyword falls through to parseExprStmt.
unique_ptr<Stmt> Parser::parseStatement() {
    TokenType t = input_.peek().getType();
    switch (t) {
        case TokenType::LBRACE:      return parseCompoundStmt();
        case TokenType::KW_IF:       return parseIfStmt();
        case TokenType::KW_WHILE:
        case TokenType::KW_DO:       return parseLoopStmt();
        case TokenType::KW_FOR:      return parseForStmt();
        case TokenType::KW_RETURN:   return parseReturnStmt();
        case TokenType::KW_BREAK:
        case TokenType::KW_CONTINUE: return parseJumpStmt();
        case TokenType::SEMICOLON:
            input_.get();        // consume the bare ';'
            return nullptr;      // empty statement carries no AST node
        default:
            return parseExprStmt();
    }
}

// Brace-delimited block. Opens a fresh scope for any declarations that
// (in C4) appear inside, delegates the item loop to parseBlockItems, then
// closes the scope on '}'.
unique_ptr<CompoundStmt> Parser::parseCompoundStmt() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    expect(TokenType::LBRACE, "expected '{' to open block");

    scopeStack_.push_back(++nextScopeId_);
    auto block = parseBlockItems(line, col);
    expect(TokenType::RBRACE, "expected '}' to close block");
    scopeStack_.pop_back();

    return block;
}

// Inner-loop helper. Builds a CompoundStmt and fills items_. Declarations
// (any token where isDeclarationStart is true) come from parseExternalDeclaration,
// which may return multiple Decls from a comma chain; each is pushed in order.
// Anything else delegates to parseStatement; nullptr returns (stray ';') are skipped.
// Does NOT push/pop scopeStack_ — that's the caller's job, so a function body
// can share the parameter scope (C99 6.2.1).
unique_ptr<CompoundStmt> Parser::parseBlockItems(int line, int col) {
    auto block = make_unique<CompoundStmt>(line, col);
    while (!input_.eof() && input_.peek().getType() != TokenType::RBRACE) {
        if (isDeclarationStart(input_.peek().getType())) {
            auto decls = parseExternalDeclaration();
            for (auto& d : decls)
                if (d) block->addItem(move(d));
        } else {
            auto stmt = parseStatement();
            if (stmt) block->addItem(move(stmt));
        }
    }
    return block;
}

// 'if (cond) then [else else]'. Dangling-else binds to the nearest unmatched
// 'if' naturally — the 'else' here only matches the most recent parseIfStmt
// invocation.
unique_ptr<IfStmt> Parser::parseIfStmt() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    expect(TokenType::KW_IF,  "expected 'if'");
    expect(TokenType::LPAREN, "expected '(' after 'if'");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "expected ')' after if condition");
    auto thenBranch = parseStatement();
    unique_ptr<Stmt> elseBranch;
    if (input_.match(TokenType::KW_ELSE)) {
        elseBranch = parseStatement();
    }
    return make_unique<IfStmt>(line, col, move(cond),
                                move(thenBranch), move(elseBranch));
}

// One method for both forms. 'while' produces isPostTest_=false; 'do' produces
// isPostTest_=true. parseStatement dispatches both KW_WHILE and KW_DO here.
unique_ptr<LoopStmt> Parser::parseLoopStmt() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();

    if (input_.peek().getType() == TokenType::KW_WHILE) {
        input_.get();   // consume 'while'
        expect(TokenType::LPAREN, "expected '(' after 'while'");
        auto cond = parseExpression();
        expect(TokenType::RPAREN, "expected ')' after while condition");
        auto body = parseStatement();
        return make_unique<LoopStmt>(line, col, /*isPostTest=*/false,
                                      move(cond), move(body));
    }

    // do-while
    expect(TokenType::KW_DO, "expected 'do'");
    auto body = parseStatement();
    expect(TokenType::KW_WHILE, "expected 'while' after do-while body");
    expect(TokenType::LPAREN,  "expected '(' after 'while'");
    auto cond = parseExpression();
    expect(TokenType::RPAREN,  "expected ')' after do-while condition");
    expect(TokenType::SEMICOLON, "expected ';' after do-while statement");
    return make_unique<LoopStmt>(line, col, /*isPostTest=*/true,
                                  move(cond), move(body));
}

// 'for (init; cond; step) body'. Any of init/cond/step may be empty.
// init accepts either the expression-statement form OR a single-declarator
// declaration form ('for (int i = 0; ...)'). The for-statement is itself a
// block (C99 6.8.5): we push a fresh scope so an init declaration is visible
// only inside the loop. Pop on exit.
unique_ptr<ForStmt> Parser::parseForStmt() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    expect(TokenType::KW_FOR, "expected 'for'");
    expect(TokenType::LPAREN, "expected '(' after 'for'");

    // For-statement scope: declarations in init are visible to cond/step/body
    // but go out of scope when the loop ends.
    scopeStack_.push_back(++nextScopeId_);

    // init clause: declaration, ExprStmt, or empty
    unique_ptr<AstNode> init;
    if (input_.peek().getType() != TokenType::SEMICOLON) {
        int initLine = input_.peek().getLine();
        int initCol  = input_.peek().getColumn();
        if (isDeclarationStart(input_.peek().getType())) {
            // Single-declarator declaration form. We don't reuse
            // parseExternalDeclaration because that would consume the trailing
            // ';' (the for-loop's separator) and would also accept a comma chain
            // — neither is right for for-init.
            Type baseType = parseDeclSpecifiers();
            Type t = baseType;
            string name;
            parseDeclarator(t, name);
            init = parseVarDecl(t, name, initLine, initCol);
        } else {
            auto initExpr = parseExpression();
            // ExprStmt for the AST shape; the trailing ';' is the for-loop
            // separator below, not part of ExprStmt's semantics.
            init = make_unique<ExprStmt>(initLine, initCol, move(initExpr));
        }
    }
    expect(TokenType::SEMICOLON, "expected ';' after for init");

    // cond clause: expression or empty
    unique_ptr<Expr> cond;
    if (input_.peek().getType() != TokenType::SEMICOLON) {
        cond = parseExpression();
    }
    expect(TokenType::SEMICOLON, "expected ';' after for condition");

    // step clause: expression or empty
    unique_ptr<Expr> step;
    if (input_.peek().getType() != TokenType::RPAREN) {
        step = parseExpression();
    }
    expect(TokenType::RPAREN, "expected ')' after for clauses");

    auto body = parseStatement();
    scopeStack_.pop_back();
    return make_unique<ForStmt>(line, col, move(init), move(cond),
                                 move(step), move(body));
}

// 'return [expr];'. value_ is null for bare 'return;'.
unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    expect(TokenType::KW_RETURN, "expected 'return'");
    unique_ptr<Expr> value;
    if (input_.peek().getType() != TokenType::SEMICOLON) {
        value = parseExpression();
    }
    expect(TokenType::SEMICOLON, "expected ';' after return");
    return make_unique<ReturnStmt>(line, col, move(value));
}

// 'break;' or 'continue;' — the dispatched-on keyword tells us which.
unique_ptr<JumpStmt> Parser::parseJumpStmt() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    TokenType kind = input_.peek().getType();   // KW_BREAK or KW_CONTINUE
    input_.get();                                // consume the keyword
    expect(TokenType::SEMICOLON, "expected ';' after break/continue");
    return make_unique<JumpStmt>(line, col, kind);
}

// 'expr;'. Position is the expression's position (start of the statement).
unique_ptr<ExprStmt> Parser::parseExprStmt() {
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "expected ';' after expression");
    return make_unique<ExprStmt>(line, col, move(expr));
}

// ===========================================================================
// Declaration parsers
// ===========================================================================

// Walks the leading specifier run, building one Type. Type-specifier keywords
// set kind_; const/volatile flip the qualifier flags; sign modifiers, redundant
// 'long', 'restrict', and storage-class keywords are consumed and discarded
// (no v1 reader for them). Reports duplicate-specifier errors when conflicting
// type specifiers are seen (with one tolerated combination: 'short int' /
// 'long int' / 'long long' all keep the modifier kind).
Type Parser::parseDeclSpecifiers() {
    Type t;
    bool kindSet = false;

    while (isDeclarationStart(input_.peek().getType())) {
        TokenType tt = input_.peek().getType();
        int   line = input_.peek().getLine();
        int   col  = input_.peek().getColumn();
        string lex = input_.peek().getLexeme();

        // Storage-class: consume and discard.
        if (isStorageClassSpecifier(tt)) {
            input_.get();
            continue;
        }

        // Qualifiers: only const/volatile are stored; restrict is discarded.
        if (isTypeQualifier(tt)) {
            input_.get();
            if (tt == TokenType::KW_CONST)         t.setConst(true);
            else if (tt == TokenType::KW_VOLATILE) t.setVolatile(true);
            // restrict: consumed, not stored
            continue;
        }

        if (isTypeSpecifier(tt)) {
            input_.get();
            // signed/unsigned: consumed, kind unchanged.
            if (tt == TokenType::KW_SIGNED || tt == TokenType::KW_UNSIGNED)
                continue;

            TypeKind newKind = TypeKind::INVALID;
            switch (tt) {
                case TokenType::KW_VOID:   newKind = TypeKind::VOID;   break;
                case TokenType::KW_CHAR:   newKind = TypeKind::CHAR;   break;
                case TokenType::KW_SHORT:  newKind = TypeKind::SHORT;  break;
                case TokenType::KW_INT:    newKind = TypeKind::INT;    break;
                case TokenType::KW_LONG:   newKind = TypeKind::LONG;   break;
                case TokenType::KW_FLOAT:  newKind = TypeKind::FLOAT;  break;
                case TokenType::KW_DOUBLE: newKind = TypeKind::DOUBLE; break;
                case TokenType::KW_BOOL:
                case TokenType::KW__BOOL:  newKind = TypeKind::BOOL;   break;
                default:                   newKind = TypeKind::INVALID; break;
            }

            if (!kindSet) {
                t.setKind(newKind);
                kindSet = true;
            } else {
                TypeKind cur = t.getKind();
                bool curIsShortOrLong = (cur == TypeKind::SHORT || cur == TypeKind::LONG);
                if (curIsShortOrLong && newKind == TypeKind::INT) {
                    // 'short int' / 'long int' — modifier wins, kind stays.
                } else if (cur == TypeKind::LONG && newKind == TypeKind::LONG) {
                    // 'long long' — second long ignored.
                } else if (cur == newKind) {
                    addError("duplicate type specifier", line, col, lex);
                } else {
                    addError("conflicting type specifier", line, col, lex);
                }
            }
            continue;
        }

        // Defensive — isDeclarationStart returned true but none of the three
        // categories matched. Bail to avoid an infinite loop.
        break;
    }
    return t;
}

// Consumes leading '*' tokens (type.addPointer() per star) then expects an
// IDENTIFIER. On a missing identifier, records an error and leaves outName empty
// so callers can decide whether to continue.
void Parser::parseDeclarator(Type& type, string& outName) {
    while (input_.match(TokenType::STAR))
        type.addPointer();

    if (input_.peek().getType() == TokenType::IDENTIFIER) {
        outName = input_.get().getLexeme();
    } else {
        const Token& bad = input_.peek();
        addError("expected identifier in declarator",
                 bad.getLine(), bad.getColumn(), bad.getLexeme());
        outName = "";
    }
}

// One top-level declaration. Parses specifiers + first declarator, then
// dispatches: '(' -> function definition (single-result); otherwise variable
// declaration with optional initializer + comma chain; trailing ';' consumed.
// Returns vector because comma chains produce multiple Decls from one production.
vector<unique_ptr<Decl>> Parser::parseExternalDeclaration() {
    vector<unique_ptr<Decl>> result;
    int line = input_.peek().getLine();
    int col  = input_.peek().getColumn();

    Type baseType = parseDeclSpecifiers();

    // First declarator
    Type firstType = baseType;
    string firstName;
    int firstLine = input_.peek().getLine();
    int firstCol  = input_.peek().getColumn();
    parseDeclarator(firstType, firstName);

    // Function definition: declarator immediately followed by '('
    if (input_.peek().getType() == TokenType::LPAREN) {
        auto fn = parseFunctionDecl(move(firstType), move(firstName), line, col);
        if (fn) result.push_back(move(fn));
        return result;
    }

    // Variable declaration; first var
    auto first = parseVarDecl(move(firstType), move(firstName), firstLine, firstCol);
    if (first) result.push_back(move(first));

    // Comma chain: each declarator restarts from baseType (resets pointer depth)
    while (input_.match(TokenType::COMMA)) {
        Type nextType = baseType;
        string nextName;
        int nextLine = input_.peek().getLine();
        int nextCol  = input_.peek().getColumn();
        parseDeclarator(nextType, nextName);
        auto v = parseVarDecl(move(nextType), move(nextName), nextLine, nextCol);
        if (v) result.push_back(move(v));
    }

    expect(TokenType::SEMICOLON, "expected ';' after declaration");
    return result;
}

// One variable: optional '= expr' initializer, then declare() at the active
// scope. Reports redeclaration on same-(scope, name). Does NOT consume ';'
// — the caller does, since multiple parseVarDecl calls in a comma chain share
// one terminator.
unique_ptr<VarDecl> Parser::parseVarDecl(Type type, string name, int line, int col) {
    unique_ptr<Expr> init;
    if (input_.match(TokenType::ASSIGN)) {
        init = parseExpression();
    }

    // Synthesize a Token from the captured name/line/col so SymbolTable::declare
    // gets a SymbolEntry built around an identifier-shaped token. SymbolEntry
    // only reads the lexeme, line, and column from its token, so this works.
    if (!name.empty()) {
        Token nameTok(TokenType::IDENTIFIER, name, line, col);
        if (!symbolTable_.declare(nameTok, scopeStack_.back(), type.toString())) {
            addError("redeclaration of '" + name + "'", line, col, name);
        }
    }

    return make_unique<VarDecl>(line, col, move(type), move(name), move(init));
}

// Function definition. Declares the function name at the OUTER scope (before
// pushing the function scope), then opens a fresh scope shared by parameters
// and body (per C99 6.2.1). Inlines the brace handling around parseBlockItems
// rather than calling parseCompoundStmt — because parseCompoundStmt would push
// yet another scope, which would isolate the body from the parameters.
unique_ptr<FunctionDecl> Parser::parseFunctionDecl(Type returnType, string name,
                                                    int line, int col) {
    // Declare the function at the outer scope.
    if (!name.empty()) {
        Token funcTok(TokenType::IDENTIFIER, name, line, col);
        if (!symbolTable_.declare(funcTok, scopeStack_.back(), returnType.toString())) {
            addError("redeclaration of '" + name + "'", line, col, name);
        }
    }

    // Function block scope: parameters and body share it.
    scopeStack_.push_back(++nextScopeId_);

    auto params = parseParamList();

    // Body must be '{' ... '}'. (Prototypes 'int f();' deferred to v2.)
    int bodyLine = input_.peek().getLine();
    int bodyCol  = input_.peek().getColumn();
    expect(TokenType::LBRACE, "expected '{' to open function body");
    auto body = parseBlockItems(bodyLine, bodyCol);
    expect(TokenType::RBRACE, "expected '}' to close function body");

    scopeStack_.pop_back();

    return make_unique<FunctionDecl>(line, col, move(returnType), move(name),
                                      move(params), move(body));
}

// '(' params ')'. Empty '()' or '(void)' returns an empty vector. Otherwise
// loops over comma-separated 'declSpec declarator' pairs, declare()'ing each
// in the active (function) scope.
vector<unique_ptr<ParamDecl>> Parser::parseParamList() {
    vector<unique_ptr<ParamDecl>> params;
    expect(TokenType::LPAREN, "expected '(' before parameter list");

    // '()' — empty list
    if (input_.peek().getType() == TokenType::RPAREN) {
        input_.get();
        return params;
    }
    // '(void)' — also empty (purely a parameter-declaration syntactic marker)
    if (input_.peek().getType() == TokenType::KW_VOID
        && input_.peek(1).getType() == TokenType::RPAREN) {
        input_.get();   // void
        input_.get();   // )
        return params;
    }

    do {
        int pLine = input_.peek().getLine();
        int pCol  = input_.peek().getColumn();
        Type pType = parseDeclSpecifiers();
        string pName;
        parseDeclarator(pType, pName);

        if (!pName.empty()) {
            Token pTok(TokenType::IDENTIFIER, pName, pLine, pCol);
            if (!symbolTable_.declare(pTok, scopeStack_.back(), pType.toString())) {
                addError("redeclaration of parameter '" + pName + "'",
                         pLine, pCol, pName);
            }
        }

        params.push_back(make_unique<ParamDecl>(pLine, pCol, move(pType), move(pName)));
    } while (input_.match(TokenType::COMMA));

    expect(TokenType::RPAREN, "expected ')' to close parameter list");
    return params;
}

// Top-level entry. Allocates the TranslationUnit and loops parseExternalDeclaration
// until EOF. On a production that returned no Decls (unrecoverable error path),
// synchronises and continues so one bad declaration doesn't abort the whole parse.
unique_ptr<TranslationUnit> Parser::parse() {
    auto tu = make_unique<TranslationUnit>(1, 1);
    while (!input_.eof()) {
        size_t before = errors_.size();
        auto decls = parseExternalDeclaration();
        for (auto& d : decls)
            if (d) tu->addDecl(move(d));
        // If nothing was produced AND no progress through expect/error, force a
        // resync to make sure we don't spin forever on garbage input.
        if (decls.empty() && errors_.size() > before) {
            synchronize();
        }
    }
    return tu;
}
