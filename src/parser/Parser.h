#pragma once
#include "TokenStream.h"
#include "ParseError.h"
#include "Token.h"
#include "SymbolTable.h"
#include "Ast.h"
#include <vector>
#include <memory>
#include <string>
using namespace std;


// The main syntactic analyzer. Consumes the lexer's token stream, builds an
// AST (added in a later phase), writes declared identifiers into the shared
// SymbolTable via declare(), and records parse errors.
//
// This file currently exposes the constructor, error accessors, and the
// private helper layer (addError/expect/synchronize) that grammar methods
// will sit on top of. Grammar methods and parse() are added in a later phase.
class Parser {
public:
    // Binds the token vector to a TokenStream, stores the symbol-table
    // reference, and initialises scope tracking (file scope = 0 is preloaded).
    Parser(const vector<Token>& tokens, SymbolTable& symbolTable);

    // Returns the list of errors accumulated so far. Same shape as Lexer::getErrors().
    const vector<ParseError>& getErrors() const;

    // Parses one expression (entry point for the expression cascade). Returns
    // an AST subtree or nullptr if the head is not the start of an expression
    // (an error is recorded and a single token is consumed for recovery).
    // Public so tests and the eventual parse() entry point can drive it
    // without needing access to private helpers.
    unique_ptr<Expr> parseExpression();

    // Parses one statement. Returns nullptr for a stray ';' (no null-statement
    // node — see Parser_Design.md §3.2.b). Public for the same reason as
    // parseExpression: tests and parse() drive it without needing private access.
    unique_ptr<Stmt> parseStatement();

    // Top-level entry point. Loops parseExternalDeclaration() until EOF,
    // adds every produced Decl to a fresh TranslationUnit, and returns it.
    // The returned TranslationUnit is always non-null; check getErrors() for
    // diagnostics. Same shape as Lexer::tokenize() at the lexer phase.
    unique_ptr<TranslationUnit> parse();

private:
    TokenStream  input_;          // token-level cursor over the lexer's output
    SymbolTable& symbolTable_;    // shared symbol table; written via declare()
    vector<ParseError> errors_;   // accumulated parse errors

    // Scope tracking for shadowing-aware declare/lookup against SymbolTable.
    // nextScopeId_ is monotonic; scopeStack_ holds the active scope chain
    // (innermost at the back). File scope occupies 0 and is always present.
    int         nextScopeId_;
    vector<int> scopeStack_;

    // Append a new ParseError to errors_. Same signature as Lexer::addError.
    void addError(const string& message, int line, int column,
                  const string& text = "");

    // If the next token's type equals expected, consume it and return true.
    // Otherwise, record an error at the current cursor (using peek() for
    // line/column/lexeme) and return false WITHOUT consuming.
    bool expect(TokenType type, const string& message);

    // Recover after a parse failure: consume tokens until the cursor sits at
    // a synchronisation point — a SEMICOLON (consumed), a brace boundary
    // (LBRACE/RBRACE, left for the caller), a statement-leading keyword
    // (KW_IF/WHILE/FOR/DO/RETURN/BREAK/CONTINUE, left for the caller), or EOF.
    void synchronize();

    // ----- Expression cascade ------------------------------------------------
    // Each method calls the next-higher-precedence method, then loops while
    // its operator class matches. Mirrors the longest-match dispatch shape
    // of Lexer::scanOperatorOrDelimiter().
    unique_ptr<Expr> parseAssignment();      // = += -= *= /= %= &= |= ^= <<= >>= (right-assoc)
    unique_ptr<Expr> parseConditional();     // ? : (right-assoc)
    unique_ptr<Expr> parseLogicalOr();       // ||
    unique_ptr<Expr> parseLogicalAnd();      // &&
    unique_ptr<Expr> parseBitwiseOr();       // |
    unique_ptr<Expr> parseBitwiseXor();      // ^
    unique_ptr<Expr> parseBitwiseAnd();      // &
    unique_ptr<Expr> parseEquality();        // == !=
    unique_ptr<Expr> parseRelational();      // < > <= >=
    unique_ptr<Expr> parseShift();           // << >>
    unique_ptr<Expr> parseAdditive();        // + -
    unique_ptr<Expr> parseMultiplicative();  // * / %
    unique_ptr<Expr> parseUnary();           // prefix unary (recurses into itself)
    unique_ptr<Expr> parsePostfix();         // postfix ++/-- and call/index chaining
    unique_ptr<Expr> parsePrimary();         // atoms: literal, identifier, ( expr )

    // ----- Statement parsers -------------------------------------------------
    // Each returns its specific Stmt subclass; parseStatement() dispatches
    // and the result implicitly converts to unique_ptr<Stmt>.
    unique_ptr<CompoundStmt> parseCompoundStmt();        // { ... }; pushes/pops scopeStack_
    unique_ptr<CompoundStmt> parseBlockItems(int line, int col);  // inner loop helper; does NOT touch scopeStack_
    unique_ptr<IfStmt>       parseIfStmt();              // if (cond) S [else S]
    unique_ptr<LoopStmt>     parseLoopStmt();            // while or do-while
    unique_ptr<ForStmt>      parseForStmt();             // for (init; cond; step) body
    unique_ptr<ReturnStmt>   parseReturnStmt();          // return [expr];
    unique_ptr<JumpStmt>     parseJumpStmt();            // break; / continue;
    unique_ptr<ExprStmt>     parseExprStmt();            // expr;

    // ----- Declaration parsers ----------------------------------------------
    // parseExternalDeclaration returns a vector because a comma-chain
    // declaration ('int a, b, c;') produces multiple Decls from one production.
    vector<unique_ptr<Decl>> parseExternalDeclaration();

    // Loops over isDeclarationStart() tokens, building one Type. Storage-class
    // keywords, sign modifiers, redundant 'long' (in 'long long'), and 'restrict'
    // are accepted syntactically but discarded. Reports duplicate-specifier errors.
    Type parseDeclSpecifiers();

    // Consumes leading '*' tokens (calling type.addPointer() for each) then
    // expects an IDENTIFIER and writes its lexeme into outName. On a missing
    // identifier, records an error and leaves outName empty.
    void parseDeclarator(Type& type, string& outName);

    // Parses a function definition. Declares the function name at the OUTER
    // scope, then pushes a fresh scope for parameters + body (shared per C99
    // 6.2.1), parses parameters via parseParamList, then '{' parseBlockItems '}',
    // and pops the scope on exit.
    unique_ptr<FunctionDecl> parseFunctionDecl(Type returnType, string name,
                                                int line, int col);

    // Parses a single variable declaration: optional '=' initializer, then
    // calls symbolTable_.declare() at the active scope. Does NOT consume the
    // trailing ';' — the caller (parseExternalDeclaration / parseForStmt) handles
    // that, since comma-chained declarations share one terminator.
    unique_ptr<VarDecl> parseVarDecl(Type type, string name, int line, int col);

    // Parses '(' params ')'. Returns empty vector for '()' or '(void)'. Each
    // parameter is declare()'d into the active scope (which is the function
    // scope established by parseFunctionDecl).
    vector<unique_ptr<ParamDecl>> parseParamList();
};
