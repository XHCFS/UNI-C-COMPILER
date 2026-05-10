#include "parser/TokenStream.h"
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "lexer/Lexer.h"
#include "Token.h"
#include "SymbolTable.h"
#include "Type.h"
#include "Ast.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
using namespace std;

static int passed = 0;
static int failed = 0;

#define ASSERT_EQ(a, b, msg) \
    if ((a) == (b)) { ++passed; } \
    else { cerr << "FAIL: " << msg << "\n  expected: " #b "\n  got: " << (a) << "\n"; ++failed; }

// Sample token sequence representing `int x = 42;` for stream-shape tests.
static vector<Token> sampleTokens() {
    return {
        Token(TokenType::KW_INT,      "int", 1, 1),
        Token(TokenType::IDENTIFIER,  "x",   1, 5),
        Token(TokenType::ASSIGN,      "=",   1, 7),
        Token(TokenType::INT_LITERAL, "42",  1, 9),
        Token(TokenType::SEMICOLON,   ";",   1, 11),
    };
}

void testPeekDoesNotConsume() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "peek returns first token");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "peek is idempotent");
    ASSERT_EQ(s.eof(),                  false,                 "peek does not advance to EOF");
}

void testPeekWithOffset() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ((int)s.peek(0).getType(), (int)TokenType::KW_INT,     "peek(0) -> KW_INT");
    ASSERT_EQ((int)s.peek(1).getType(), (int)TokenType::IDENTIFIER, "peek(1) -> IDENTIFIER");
    ASSERT_EQ(s.peek(2).getLexeme(),    string("="),           "peek(2) lexeme '='");
    // Past-end peek returns the sentinel (INVALID).
    ASSERT_EQ((int)s.peek(99).getType(), (int)TokenType::INVALID,   "peek past end -> sentinel");
}

void testGetAdvances() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    Token t0 = s.get();
    ASSERT_EQ((int)t0.getType(), (int)TokenType::KW_INT,     "get returns first token");
    Token t1 = s.get();
    ASSERT_EQ((int)t1.getType(), (int)TokenType::IDENTIFIER, "get advances to second");
    ASSERT_EQ(s.eof(),           false,                      "still not EOF after 2 gets");
}

void testEofAfterAllConsumed() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    for (size_t i = 0; i < toks.size(); ++i) (void)s.get();
    ASSERT_EQ(s.eof(), true, "EOF after consuming all 5 tokens");
    // get() past end keeps returning the sentinel without advancing further.
    Token sentinel = s.get();
    ASSERT_EQ((int)sentinel.getType(), (int)TokenType::INVALID, "get past end -> sentinel type");
    ASSERT_EQ(sentinel.getLexeme(),    string(""),         "sentinel lexeme empty");
    ASSERT_EQ(s.eof(),                 true,                    "eof stays true past end");
}

void testEofSentinelLineColFromLastToken() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    while (!s.eof()) (void)s.get();
    // Sentinel inherits line/col from the last real token (';' at L1:C11).
    Token sentinel = s.peek();
    ASSERT_EQ(sentinel.getLine(),   1,  "sentinel line copied from last token");
    ASSERT_EQ(sentinel.getColumn(), 11, "sentinel column copied from last token");
}

void testEofSentinelEmptyStream() {
    vector<Token> empty;
    TokenStream s(empty);
    ASSERT_EQ(s.eof(), true, "empty stream is EOF immediately");
    Token sentinel = s.peek();
    ASSERT_EQ((int)sentinel.getType(), (int)TokenType::INVALID, "empty stream sentinel type");
    ASSERT_EQ(sentinel.getLine(),   1, "empty stream sentinel line falls back to 1");
    ASSERT_EQ(sentinel.getColumn(), 1, "empty stream sentinel column falls back to 1");
}

void testGetLineColumnTracking() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ(s.getLine(),   1, "getLine of next token");
    ASSERT_EQ(s.getColumn(), 1, "getColumn of next token");
    s.get();
    ASSERT_EQ(s.getColumn(), 5, "getColumn moves to second token after one get");
    // At EOF, getLine/getColumn fall through to the sentinel.
    while (!s.eof()) (void)s.get();
    ASSERT_EQ(s.getLine(),   1,  "getLine at EOF -> sentinel line");
    ASSERT_EQ(s.getColumn(), 11, "getColumn at EOF -> sentinel column");
}

void testMatchConsumesOnHit() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    bool ok = s.match(TokenType::KW_INT);
    ASSERT_EQ(ok, true, "match KW_INT returns true");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::IDENTIFIER, "cursor advanced after match");
}

void testMatchLeavesCursorOnMiss() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    bool ok = s.match(TokenType::IDENTIFIER);
    ASSERT_EQ(ok, false, "match IDENTIFIER fails (head is KW_INT)");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "cursor unchanged after mismatch");
}

void testMatchAtEofIsFalse() {
    vector<Token> empty;
    TokenStream s(empty);
    ASSERT_EQ(s.match(TokenType::SEMICOLON), false, "match at EOF returns false");
    ASSERT_EQ(s.eof(),                       true,  "eof unchanged after match at EOF");
}

void testCheckIsPeekOnly() {
    auto toks = sampleTokens();
    TokenStream s(toks);
    ASSERT_EQ(s.check(TokenType::KW_INT),     true,  "check KW_INT true at head");
    ASSERT_EQ(s.check(TokenType::IDENTIFIER), false, "check IDENTIFIER false at head");
    ASSERT_EQ((int)s.peek().getType(), (int)TokenType::KW_INT, "check did not advance");
}

// ---------- ParseError ----------

void testParseErrorGetters() {
    ParseError e("expected ';'", 3, 14, ";missing");
    ASSERT_EQ(e.getMessage(), string("expected ';'"), "getMessage returns plain message");
    ASSERT_EQ(e.getLine(),    3,                            "getLine returns line");
    ASSERT_EQ(e.getColumn(),  14,                           "getColumn returns column");
}

void testParseErrorToStringWithText() {
    ParseError e("expected ';'", 3, 14, "}");
    ASSERT_EQ(e.toString(),
              string("ParseError [3:14] expected ';' (\"}\")"),
              "toString includes prefix, location, message, and quoted offending text");
}

void testParseErrorToStringWithoutText() {
    ParseError e("unexpected end of file", 7, 1);
    ASSERT_EQ(e.toString(),
              string("ParseError [7:1] unexpected end of file"),
              "toString omits parens when offendingText is empty");
}

void testParseErrorDefaultOffendingText() {
    // The default-arg form should be equivalent to passing an empty string.
    ParseError e("missing return type", 1, 1);
    ASSERT_EQ(e.toString(),
              string("ParseError [1:1] missing return type"),
              "default-arg ctor produces same toString as explicit empty text");
}

// ---------- Parser construction ----------

void testParserConstructsWithEmptyStream() {
    SymbolTable st;
    vector<Token> empty;
    Parser p(empty, st);
    ASSERT_EQ(p.getErrors().empty(), true, "fresh parser has no errors");
}

void testParserConstructsWithTokens() {
    SymbolTable st;
    auto toks = sampleTokens();
    Parser p(toks, st);
    ASSERT_EQ(p.getErrors().empty(), true, "no errors before any parsing");
    ASSERT_EQ(p.getErrors().size(),  (size_t)0, "errors list starts empty");
}

// ---------- Type ----------

void testTypeDefaults() {
    Type t;
    ASSERT_EQ((int)t.getKind(),     (int)TypeKind::INVALID, "default Type is INVALID");
    ASSERT_EQ(t.getPointerDepth(),  0,                      "default pointer depth is 0");
    ASSERT_EQ(t.isConst(),          false,                  "default is not const");
    ASSERT_EQ(t.isVolatile(),       false,                  "default is not volatile");
}

void testTypeKindCtor() {
    Type t(TypeKind::INT);
    ASSERT_EQ((int)t.getKind(),     (int)TypeKind::INT, "kind ctor sets kind");
    ASSERT_EQ(t.getPointerDepth(),  0,                  "kind ctor leaves pointer depth 0");
    ASSERT_EQ(t.isConst(),          false,              "kind ctor leaves const false");
}

void testTypeMutators() {
    Type t;
    t.setKind(TypeKind::FLOAT);
    t.addPointer();
    t.addPointer();
    t.setConst(true);
    t.setVolatile(true);
    ASSERT_EQ((int)t.getKind(),     (int)TypeKind::FLOAT, "setKind");
    ASSERT_EQ(t.getPointerDepth(),  2,                    "addPointer twice -> depth 2");
    ASSERT_EQ(t.isConst(),          true,                 "setConst(true)");
    ASSERT_EQ(t.isVolatile(),       true,                 "setVolatile(true)");
}

void testTypeToString() {
    Type plain(TypeKind::INT);
    ASSERT_EQ(plain.toString(), string("int"), "plain int");

    Type ptr(TypeKind::CHAR);
    ptr.addPointer();
    ASSERT_EQ(ptr.toString(), string("char*"), "char*");

    Type ptr2(TypeKind::INT);
    ptr2.addPointer();
    ptr2.addPointer();
    ASSERT_EQ(ptr2.toString(), string("int**"), "int**");

    Type cnst(TypeKind::INT);
    cnst.setConst(true);
    ASSERT_EQ(cnst.toString(), string("const int"), "const int");

    Type cv(TypeKind::DOUBLE);
    cv.setConst(true);
    cv.setVolatile(true);
    cv.addPointer();
    ASSERT_EQ(cv.toString(), string("const volatile double*"), "const volatile double*");
}

void testTypeKindToString() {
    ASSERT_EQ(typeKindToString(TypeKind::VOID),    string("void"),    "void");
    ASSERT_EQ(typeKindToString(TypeKind::CHAR),    string("char"),    "char");
    ASSERT_EQ(typeKindToString(TypeKind::SHORT),   string("short"),   "short");
    ASSERT_EQ(typeKindToString(TypeKind::INT),     string("int"),     "int");
    ASSERT_EQ(typeKindToString(TypeKind::LONG),    string("long"),    "long");
    ASSERT_EQ(typeKindToString(TypeKind::FLOAT),   string("float"),   "float");
    ASSERT_EQ(typeKindToString(TypeKind::DOUBLE),  string("double"),  "double");
    ASSERT_EQ(typeKindToString(TypeKind::BOOL),    string("bool"),    "bool");
    ASSERT_EQ(typeKindToString(TypeKind::INVALID), string("invalid"), "invalid");
}

// ---------- AST construction + getters ----------

void testLiteralExprGettersAndDump() {
    LiteralExpr e(1, 1, TokenType::INT_LITERAL, "42");
    ASSERT_EQ((int)e.getKind(),     (int)TokenType::INT_LITERAL, "kind");
    ASSERT_EQ(e.getLexeme(),        string("42"),                "lexeme");
    ASSERT_EQ(e.getLine(),          1,                           "line");
    ASSERT_EQ(e.getColumn(),        1,                           "column");

    ostringstream oss;
    e.dump(oss, 0);
    ASSERT_EQ(oss.str(), string("LiteralExpr INT_LITERAL '42'\n"), "literal dump format");
}

void testIdentifierExprGettersAndDump() {
    IdentifierExpr e(2, 5, "myVar");
    ASSERT_EQ(e.getName(),  string("myVar"), "name");
    ASSERT_EQ(e.getLine(),  2,                "line");
    ASSERT_EQ(e.getColumn(),5,                "column");

    ostringstream oss;
    e.dump(oss, 0);
    ASSERT_EQ(oss.str(), string("IdentifierExpr 'myVar'\n"), "identifier dump format");
}

// Build `a + 1` and check the indented dump structure.
void testBinaryExprNestedDump() {
    auto lhs = make_unique<IdentifierExpr>(1, 1, "a");
    auto rhs = make_unique<LiteralExpr>(1, 5, TokenType::INT_LITERAL, "1");
    BinaryExpr bin(1, 3, TokenType::PLUS, move(lhs), move(rhs));

    ASSERT_EQ((int)bin.getOp(), (int)TokenType::PLUS, "binary op");
    ASSERT_EQ(bin.getLhs() != nullptr, true, "binary lhs non-null");
    ASSERT_EQ(bin.getRhs() != nullptr, true, "binary rhs non-null");

    ostringstream oss;
    bin.dump(oss, 0);
    ASSERT_EQ(oss.str(),
              string("BinaryExpr PLUS\n"
                     "  IdentifierExpr 'a'\n"
                     "  LiteralExpr INT_LITERAL '1'\n"),
              "binary expr nested dump");
}

// Polymorphic dispatch: invoking dump() through AstNode* must hit the concrete override.
void testPolymorphicDispatch() {
    unique_ptr<AstNode> node = make_unique<IdentifierExpr>(1, 1, "x");
    ostringstream oss;
    node->dump(oss, 0);
    ASSERT_EQ(oss.str(), string("IdentifierExpr 'x'\n"), "AstNode* dispatch hits concrete dump");
}

// UnaryExpr distinguishes prefix from postfix in its dump output.
void testUnaryPrefixVsPostfix() {
    auto opnd1 = make_unique<IdentifierExpr>(1, 1, "i");
    UnaryExpr pre(1, 1, false, TokenType::INCREMENT, move(opnd1));
    ostringstream oss1;
    pre.dump(oss1, 0);
    ASSERT_EQ(oss1.str(),
              string("UnaryExpr INCREMENT prefix\n  IdentifierExpr 'i'\n"),
              "prefix unary dump");

    auto opnd2 = make_unique<IdentifierExpr>(1, 1, "i");
    UnaryExpr post(1, 1, true, TokenType::INCREMENT, move(opnd2));
    ostringstream oss2;
    post.dump(oss2, 0);
    ASSERT_EQ(oss2.str(),
              string("UnaryExpr INCREMENT postfix\n  IdentifierExpr 'i'\n"),
              "postfix unary dump");
}

// LoopStmt dump distinguishes while from do-while via the isPostTest_ flag.
void testLoopStmtWhileVsDoWhile() {
    auto cond1 = make_unique<LiteralExpr>(1, 1, TokenType::INT_LITERAL, "1");
    LoopStmt w(1, 1, false, move(cond1), nullptr);
    ostringstream oss1;
    w.dump(oss1, 0);
    ASSERT_EQ(oss1.str(),
              string("LoopStmt while\n  LiteralExpr INT_LITERAL '1'\n"),
              "while loop dump");

    auto cond2 = make_unique<LiteralExpr>(1, 1, TokenType::INT_LITERAL, "1");
    LoopStmt dw(1, 1, true, move(cond2), nullptr);
    ostringstream oss2;
    dw.dump(oss2, 0);
    ASSERT_EQ(oss2.str(),
              string("LoopStmt do-while\n  LiteralExpr INT_LITERAL '1'\n"),
              "do-while loop dump");
}

// IfStmt tolerates null then/else (the empty-body case after dropping NullStmt).
void testIfStmtNullableBranches() {
    auto cond = make_unique<IdentifierExpr>(1, 1, "c");
    IfStmt iff(1, 1, move(cond), nullptr, nullptr);
    ostringstream oss;
    iff.dump(oss, 0);
    ASSERT_EQ(oss.str(),
              string("IfStmt\n  IdentifierExpr 'c'\n"),
              "IfStmt with null then/else still prints cleanly");
}

// VarDecl dump shows name and resolved type via Type::toString().
void testVarDeclDump() {
    Type t(TypeKind::INT);
    auto init = make_unique<LiteralExpr>(1, 1, TokenType::INT_LITERAL, "10");
    VarDecl v(1, 1, t, "x", move(init));
    ASSERT_EQ(v.getName(),       string("x"), "VarDecl name");
    ASSERT_EQ(v.getType().toString(), string("int"), "VarDecl type round-trips");

    ostringstream oss;
    v.dump(oss, 0);
    ASSERT_EQ(oss.str(),
              string("VarDecl 'x' int\n  LiteralExpr INT_LITERAL '10'\n"),
              "VarDecl dump with initializer");
}

// JumpStmt dump shows break vs continue via TokenType::tokenTypeToString.
void testJumpStmtDump() {
    JumpStmt brk(1, 1, TokenType::KW_BREAK);
    ostringstream oss1;
    brk.dump(oss1, 0);
    ASSERT_EQ(oss1.str(), string("JumpStmt KW_BREAK\n"), "break jump dump");

    JumpStmt cnt(1, 1, TokenType::KW_CONTINUE);
    ostringstream oss2;
    cnt.dump(oss2, 0);
    ASSERT_EQ(oss2.str(), string("JumpStmt KW_CONTINUE\n"), "continue jump dump");
}

// ---------- Expression cascade ----------
// These tests tokenize a source snippet via the real Lexer, feed it to
// Parser::parseExpression(), and compare the dump() output against the
// expected indented tree shape.

static string parseExprDump(const string& src) {
    SymbolTable st;
    Lexer lex(src, st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    auto e = p.parseExpression();
    ostringstream oss;
    if (e) e->dump(oss, 0);
    return oss.str();
}

static int parseExprErrorCount(const string& src) {
    SymbolTable st;
    Lexer lex(src, st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    (void)p.parseExpression();
    return (int)p.getErrors().size();
}

void testExprPrimaryLiteral() {
    ASSERT_EQ(parseExprDump("42"),
              string("LiteralExpr INT_LITERAL '42'\n"),
              "primary int literal");
}

void testExprPrimaryIdentifier() {
    ASSERT_EQ(parseExprDump("foo"),
              string("IdentifierExpr 'foo'\n"),
              "primary identifier");
}

void testExprParensNoWrapper() {
    // Parens are structural — the inner expression is returned without a
    // dedicated ParenExpr node.
    ASSERT_EQ(parseExprDump("(42)"),
              string("LiteralExpr INT_LITERAL '42'\n"),
              "parenthesised primary unwraps");
}

void testExprBinaryAddition() {
    ASSERT_EQ(parseExprDump("a + b"),
              string("BinaryExpr PLUS\n"
                     "  IdentifierExpr 'a'\n"
                     "  IdentifierExpr 'b'\n"),
              "a + b");
}

void testExprPrecedenceMulOverAdd() {
    // a + b * c -> a + (b * c)
    ASSERT_EQ(parseExprDump("a + b * c"),
              string("BinaryExpr PLUS\n"
                     "  IdentifierExpr 'a'\n"
                     "  BinaryExpr STAR\n"
                     "    IdentifierExpr 'b'\n"
                     "    IdentifierExpr 'c'\n"),
              "multiplicative binds tighter than additive");
}

void testExprLeftAssociativity() {
    // a - b - c -> (a - b) - c
    ASSERT_EQ(parseExprDump("a - b - c"),
              string("BinaryExpr MINUS\n"
                     "  BinaryExpr MINUS\n"
                     "    IdentifierExpr 'a'\n"
                     "    IdentifierExpr 'b'\n"
                     "  IdentifierExpr 'c'\n"),
              "additive is left-associative");
}

void testExprPrefixUnary() {
    ASSERT_EQ(parseExprDump("-a"),
              string("UnaryExpr MINUS prefix\n"
                     "  IdentifierExpr 'a'\n"),
              "prefix minus");
}

void testExprPostfixIncrement() {
    ASSERT_EQ(parseExprDump("a++"),
              string("UnaryExpr INCREMENT postfix\n"
                     "  IdentifierExpr 'a'\n"),
              "postfix ++");
}

void testExprUnaryAndAdditiveCompose() {
    // -a + b -> (-a) + b  (unary tighter than additive)
    ASSERT_EQ(parseExprDump("-a + b"),
              string("BinaryExpr PLUS\n"
                     "  UnaryExpr MINUS prefix\n"
                     "    IdentifierExpr 'a'\n"
                     "  IdentifierExpr 'b'\n"),
              "unary tighter than additive");
}

void testExprAssignmentRightAssoc() {
    // a = b = c -> a = (b = c)
    ASSERT_EQ(parseExprDump("a = b = c"),
              string("AssignExpr ASSIGN\n"
                     "  IdentifierExpr 'a'\n"
                     "  AssignExpr ASSIGN\n"
                     "    IdentifierExpr 'b'\n"
                     "    IdentifierExpr 'c'\n"),
              "assignment is right-associative");
}

void testExprCompoundAssignment() {
    ASSERT_EQ(parseExprDump("a += 1"),
              string("AssignExpr PLUS_ASSIGN\n"
                     "  IdentifierExpr 'a'\n"
                     "  LiteralExpr INT_LITERAL '1'\n"),
              "compound assignment uses PLUS_ASSIGN");
}

void testExprConditional() {
    // a ? b : c -> ConditionalExpr (right-assoc with chained ?: would also nest,
    // but the simple non-chained form already verifies the production).
    ASSERT_EQ(parseExprDump("a ? b : c"),
              string("ConditionalExpr\n"
                     "  IdentifierExpr 'a'\n"
                     "  IdentifierExpr 'b'\n"
                     "  IdentifierExpr 'c'\n"),
              "ternary ?:");
}

void testExprLogicalPrecedence() {
    // a || b && c -> a || (b && c)
    ASSERT_EQ(parseExprDump("a || b && c"),
              string("BinaryExpr LOGICAL_OR\n"
                     "  IdentifierExpr 'a'\n"
                     "  BinaryExpr LOGICAL_AND\n"
                     "    IdentifierExpr 'b'\n"
                     "    IdentifierExpr 'c'\n"),
              "&& binds tighter than ||");
}

void testExprBitwisePrecedence() {
    // a | b & c -> a | (b & c)
    ASSERT_EQ(parseExprDump("a | b & c"),
              string("BinaryExpr BITWISE_OR\n"
                     "  IdentifierExpr 'a'\n"
                     "  BinaryExpr BITWISE_AND\n"
                     "    IdentifierExpr 'b'\n"
                     "    IdentifierExpr 'c'\n"),
              "& binds tighter than |");
}

void testExprShiftAndAdditive() {
    // a << b + c -> a << (b + c)  (additive higher precedence than shift)
    ASSERT_EQ(parseExprDump("a << b + c"),
              string("BinaryExpr LSHIFT\n"
                     "  IdentifierExpr 'a'\n"
                     "  BinaryExpr PLUS\n"
                     "    IdentifierExpr 'b'\n"
                     "    IdentifierExpr 'c'\n"),
              "additive tighter than shift");
}

void testExprCallEmpty() {
    ASSERT_EQ(parseExprDump("f()"),
              string("CallExpr\n"
                     "  IdentifierExpr 'f'\n"),
              "call with no args");
}

void testExprCallWithArgs() {
    ASSERT_EQ(parseExprDump("f(a, b)"),
              string("CallExpr\n"
                     "  IdentifierExpr 'f'\n"
                     "  IdentifierExpr 'a'\n"
                     "  IdentifierExpr 'b'\n"),
              "call with two args");
}

void testExprIndex() {
    ASSERT_EQ(parseExprDump("a[i]"),
              string("IndexExpr\n"
                     "  IdentifierExpr 'a'\n"
                     "  IdentifierExpr 'i'\n"),
              "subscript a[i]");
}

void testExprChainedPostfix() {
    // f(a)[b]++ -> ((f(a))[b])++
    ASSERT_EQ(parseExprDump("f(a)[b]++"),
              string("UnaryExpr INCREMENT postfix\n"
                     "  IndexExpr\n"
                     "    CallExpr\n"
                     "      IdentifierExpr 'f'\n"
                     "      IdentifierExpr 'a'\n"
                     "    IdentifierExpr 'b'\n"),
              "chained postfix call+index+increment");
}

void testExprErrorOnBareOperator() {
    // Bare '+' has no operand. parsePrimary reports "expected expression"
    // for the missing rhs; the parser doesn't crash.
    ASSERT_EQ(parseExprErrorCount("+") >= 1, true,
              "bare '+' produces at least one parse error");
}

// ---------- Statement parsers ----------

static string parseStmtDump(const string& src) {
    SymbolTable st;
    Lexer lex(src, st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    auto s = p.parseStatement();
    ostringstream oss;
    if (s) s->dump(oss, 0);
    return oss.str();
}

static int parseStmtErrorCount(const string& src) {
    SymbolTable st;
    Lexer lex(src, st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    (void)p.parseStatement();
    return (int)p.getErrors().size();
}

void testStmtBareSemicolonReturnsNull() {
    SymbolTable st;
    Lexer lex(";", st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    auto s = p.parseStatement();
    ASSERT_EQ(s == nullptr, true, "stray ';' returns nullptr (no NullStmt)");
}

void testStmtExprStmt() {
    ASSERT_EQ(parseStmtDump("x + 1;"),
              string("ExprStmt\n"
                     "  BinaryExpr PLUS\n"
                     "    IdentifierExpr 'x'\n"
                     "    LiteralExpr INT_LITERAL '1'\n"),
              "expression statement");
}

void testStmtReturnNoValue() {
    ASSERT_EQ(parseStmtDump("return;"),
              string("ReturnStmt\n"),
              "bare return; has no value child");
}

void testStmtReturnWithValue() {
    ASSERT_EQ(parseStmtDump("return 0;"),
              string("ReturnStmt\n"
                     "  LiteralExpr INT_LITERAL '0'\n"),
              "return 0;");
}

void testStmtBreakAndContinue() {
    ASSERT_EQ(parseStmtDump("break;"),
              string("JumpStmt KW_BREAK\n"),
              "break;");
    ASSERT_EQ(parseStmtDump("continue;"),
              string("JumpStmt KW_CONTINUE\n"),
              "continue;");
}

void testStmtIfNoElse() {
    ASSERT_EQ(parseStmtDump("if (x) y;"),
              string("IfStmt\n"
                     "  IdentifierExpr 'x'\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'y'\n"),
              "if without else");
}

void testStmtIfElse() {
    ASSERT_EQ(parseStmtDump("if (x) y; else z;"),
              string("IfStmt\n"
                     "  IdentifierExpr 'x'\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'y'\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'z'\n"),
              "if with else");
}

void testStmtIfDanglingElse() {
    // 'else' binds to the nearest unmatched 'if' — the inner one.
    ASSERT_EQ(parseStmtDump("if (a) if (b) c; else d;"),
              string("IfStmt\n"
                     "  IdentifierExpr 'a'\n"
                     "  IfStmt\n"
                     "    IdentifierExpr 'b'\n"
                     "    ExprStmt\n"
                     "      IdentifierExpr 'c'\n"
                     "    ExprStmt\n"
                     "      IdentifierExpr 'd'\n"),
              "dangling else binds to inner if");
}

void testStmtIfEmptyBody() {
    // Empty body is represented as nullptr (no NullStmt). The dumper skips it.
    ASSERT_EQ(parseStmtDump("if (x) ;"),
              string("IfStmt\n"
                     "  IdentifierExpr 'x'\n"),
              "if with bare-';' body has null then_");
}

void testStmtWhile() {
    ASSERT_EQ(parseStmtDump("while (x < 10) x++;"),
              string("LoopStmt while\n"
                     "  BinaryExpr LT\n"
                     "    IdentifierExpr 'x'\n"
                     "    LiteralExpr INT_LITERAL '10'\n"
                     "  ExprStmt\n"
                     "    UnaryExpr INCREMENT postfix\n"
                     "      IdentifierExpr 'x'\n"),
              "while loop");
}

void testStmtDoWhile() {
    ASSERT_EQ(parseStmtDump("do x++; while (x < 10);"),
              string("LoopStmt do-while\n"
                     "  BinaryExpr LT\n"
                     "    IdentifierExpr 'x'\n"
                     "    LiteralExpr INT_LITERAL '10'\n"
                     "  ExprStmt\n"
                     "    UnaryExpr INCREMENT postfix\n"
                     "      IdentifierExpr 'x'\n"),
              "do-while loop");
}

void testStmtFor() {
    ASSERT_EQ(parseStmtDump("for (i = 0; i < 10; i = i + 1) x;"),
              string("ForStmt\n"
                     "  ExprStmt\n"
                     "    AssignExpr ASSIGN\n"
                     "      IdentifierExpr 'i'\n"
                     "      LiteralExpr INT_LITERAL '0'\n"
                     "  BinaryExpr LT\n"
                     "    IdentifierExpr 'i'\n"
                     "    LiteralExpr INT_LITERAL '10'\n"
                     "  AssignExpr ASSIGN\n"
                     "    IdentifierExpr 'i'\n"
                     "    BinaryExpr PLUS\n"
                     "      IdentifierExpr 'i'\n"
                     "      LiteralExpr INT_LITERAL '1'\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'x'\n"),
              "for loop with all clauses");
}

void testStmtForAllEmpty() {
    // for (;;) x;  — every clause null, body present
    ASSERT_EQ(parseStmtDump("for (;;) x;"),
              string("ForStmt\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'x'\n"),
              "for (;;) with empty clauses dumps only body");
}

void testStmtCompoundEmpty() {
    ASSERT_EQ(parseStmtDump("{}"),
              string("CompoundStmt\n"),
              "empty compound");
}

void testStmtCompoundMultipleItems() {
    ASSERT_EQ(parseStmtDump("{ x; y; }"),
              string("CompoundStmt\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'x'\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'y'\n"),
              "compound with two statements");
}

void testStmtCompoundSkipsStraySemis() {
    // Stray ';' tokens between items should NOT add anything to items_.
    ASSERT_EQ(parseStmtDump("{ ; x; ; y; ; }"),
              string("CompoundStmt\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'x'\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'y'\n"),
              "compound silently skips stray ';'");
}

void testStmtNestedCompound() {
    ASSERT_EQ(parseStmtDump("{ { x; } y; }"),
              string("CompoundStmt\n"
                     "  CompoundStmt\n"
                     "    ExprStmt\n"
                     "      IdentifierExpr 'x'\n"
                     "  ExprStmt\n"
                     "    IdentifierExpr 'y'\n"),
              "nested compound");
}

void testStmtErrorMissingRParen() {
    // 'if (x y;'  — missing ')' after condition; expect at least one error.
    ASSERT_EQ(parseStmtErrorCount("if (x y;") >= 1, true,
              "missing ')' in if-condition reports an error");
}

void testStmtErrorMissingSemiAfterReturn() {
    // 'return 0' (no semicolon) — expect at least one error.
    ASSERT_EQ(parseStmtErrorCount("return 0") >= 1, true,
              "missing ';' after return reports an error");
}

// ---------- C4: declarations + parse() entry ----------

// Helper: tokenize -> parse() -> dump the TranslationUnit.
static string parseTuDump(const string& src) {
    SymbolTable st;
    Lexer lex(src, st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    auto tu = p.parse();
    ostringstream oss;
    if (tu) tu->dump(oss, 0);
    return oss.str();
}

// Helper: error count after parse().
static int parseTuErrorCount(const string& src) {
    SymbolTable st;
    Lexer lex(src, st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    (void)p.parse();
    return (int)p.getErrors().size();
}

void testParseEmptyTu() {
    ASSERT_EQ(parseTuDump(""),
              string("TranslationUnit\n"),
              "empty source -> empty TU");
}

void testParseSingleVarDecl() {
    ASSERT_EQ(parseTuDump("int x;"),
              string("TranslationUnit\n"
                     "  VarDecl 'x' int\n"),
              "single int var");
}

void testParseVarDeclWithInit() {
    ASSERT_EQ(parseTuDump("int x = 5;"),
              string("TranslationUnit\n"
                     "  VarDecl 'x' int\n"
                     "    LiteralExpr INT_LITERAL '5'\n"),
              "var decl with initializer");
}

void testParseCommaChain() {
    ASSERT_EQ(parseTuDump("int a, b, c;"),
              string("TranslationUnit\n"
                     "  VarDecl 'a' int\n"
                     "  VarDecl 'b' int\n"
                     "  VarDecl 'c' int\n"),
              "comma chain produces three VarDecls");
}

void testParseCommaChainPointerResetsPerDeclarator() {
    // 'int *p, q;' — p is int*, q is int (pointer is per-declarator)
    ASSERT_EQ(parseTuDump("int *p, q;"),
              string("TranslationUnit\n"
                     "  VarDecl 'p' int*\n"
                     "  VarDecl 'q' int\n"),
              "pointer level resets between comma-chained declarators");
}

void testParseConstQualifier() {
    ASSERT_EQ(parseTuDump("const int x = 5;"),
              string("TranslationUnit\n"
                     "  VarDecl 'x' const int\n"
                     "    LiteralExpr INT_LITERAL '5'\n"),
              "const int from qualifier + specifier");
}

void testParseStorageClassDiscarded() {
    // 'static' is consumed but not stored on Type.
    ASSERT_EQ(parseTuDump("static int x;"),
              string("TranslationUnit\n"
                     "  VarDecl 'x' int\n"),
              "static is consumed but not surfaced on Type");
}

void testParseUnsignedDiscarded() {
    // 'unsigned int' -> kind INT (sign modifier discarded).
    ASSERT_EQ(parseTuDump("unsigned int x;"),
              string("TranslationUnit\n"
                     "  VarDecl 'x' int\n"),
              "unsigned modifier discarded");
}

void testParseLongLong() {
    // 'long long' -> kind LONG (second long ignored).
    ASSERT_EQ(parseTuDump("long long x;"),
              string("TranslationUnit\n"
                     "  VarDecl 'x' long\n"),
              "long long collapses to long");
}

void testParseFunctionEmptyParams() {
    ASSERT_EQ(parseTuDump("int main() { return 0; }"),
              string("TranslationUnit\n"
                     "  FunctionDecl 'main' returns int\n"
                     "    CompoundStmt\n"
                     "      ReturnStmt\n"
                     "        LiteralExpr INT_LITERAL '0'\n"),
              "function with empty parameter list");
}

void testParseFunctionVoidParams() {
    ASSERT_EQ(parseTuDump("int f(void) { return 0; }"),
              string("TranslationUnit\n"
                     "  FunctionDecl 'f' returns int\n"
                     "    CompoundStmt\n"
                     "      ReturnStmt\n"
                     "        LiteralExpr INT_LITERAL '0'\n"),
              "(void) parameter list is empty");
}

void testParseFunctionNamedParams() {
    ASSERT_EQ(parseTuDump("int add(int a, int b) { return a + b; }"),
              string("TranslationUnit\n"
                     "  FunctionDecl 'add' returns int\n"
                     "    ParamDecl 'a' int\n"
                     "    ParamDecl 'b' int\n"
                     "    CompoundStmt\n"
                     "      ReturnStmt\n"
                     "        BinaryExpr PLUS\n"
                     "          IdentifierExpr 'a'\n"
                     "          IdentifierExpr 'b'\n"),
              "function with two named parameters");
}

void testParseDeclarationInCompound() {
    ASSERT_EQ(parseTuDump("int main() { int x; x = 5; return x; }"),
              string("TranslationUnit\n"
                     "  FunctionDecl 'main' returns int\n"
                     "    CompoundStmt\n"
                     "      VarDecl 'x' int\n"
                     "      ExprStmt\n"
                     "        AssignExpr ASSIGN\n"
                     "          IdentifierExpr 'x'\n"
                     "          LiteralExpr INT_LITERAL '5'\n"
                     "      ReturnStmt\n"
                     "        IdentifierExpr 'x'\n"),
              "declarations interleave with statements inside compound");
}

void testParseForDeclarationInit() {
    ASSERT_EQ(parseTuDump("int main() { for (int i = 0; i < 10; i = i + 1) ; }"),
              string("TranslationUnit\n"
                     "  FunctionDecl 'main' returns int\n"
                     "    CompoundStmt\n"
                     "      ForStmt\n"
                     "        VarDecl 'i' int\n"
                     "          LiteralExpr INT_LITERAL '0'\n"
                     "        BinaryExpr LT\n"
                     "          IdentifierExpr 'i'\n"
                     "          LiteralExpr INT_LITERAL '10'\n"
                     "        AssignExpr ASSIGN\n"
                     "          IdentifierExpr 'i'\n"
                     "          BinaryExpr PLUS\n"
                     "            IdentifierExpr 'i'\n"
                     "            LiteralExpr INT_LITERAL '1'\n"),
              "for-init declaration form supported");
}

// ---------- Symbol-table integration ----------

void testSymbolTableFileScopeVar() {
    SymbolTable st;
    Lexer lex("int x;", st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    (void)p.parse();
    SymbolEntry* e = st.find("x", 0);
    ASSERT_EQ(e != nullptr,                  true,           "x declared at file scope");
    ASSERT_EQ(e->getDataType(), string("int"),                 "x has dataType 'int'");
    ASSERT_EQ(e->getScope(),                 0,              "x at scope 0");
}

void testSymbolTableFunctionAndParamsAndLocal() {
    SymbolTable st;
    Lexer lex("int add(int a, int b) { int c; return c; }", st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    (void)p.parse();
    // Function name at file scope
    SymbolEntry* f = st.find("add", 0);
    ASSERT_EQ(f != nullptr,                       true,            "add declared at file scope");
    ASSERT_EQ(f->getDataType(),    string("int"),                  "add returns int");

    // Parameters at function scope (1) — shared with the body
    SymbolEntry* a = st.find("a", 1);
    SymbolEntry* b = st.find("b", 1);
    ASSERT_EQ(a != nullptr,                       true,            "param a at scope 1");
    ASSERT_EQ(b != nullptr,                       true,            "param b at scope 1");

    // Local 'c' in the same scope as parameters (function body shares the scope)
    SymbolEntry* c = st.find("c", 1);
    ASSERT_EQ(c != nullptr,                       true,            "local c at scope 1 (shared with params)");
}

void testSymbolTableShadowingThroughNestedScopes() {
    // int x at file scope; function 'main' opens scope 1; an inner block opens
    // scope 2 with another 'int x'. The two x's coexist as (0,"x") and (2,"x").
    SymbolTable st;
    Lexer lex("int x; int main() { { int x; } return 0; }", st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    (void)p.parse();
    int countOfX = 0;
    int seenScope0 = 0, seenScope2 = 0;
    for (const auto& [key, entry] : st.getAllEntries()) {
        if (key.name == "x" && key.scope >= 0) {
            ++countOfX;
            if (key.scope == 0) ++seenScope0;
            if (key.scope == 2) ++seenScope2;
        }
    }
    ASSERT_EQ(countOfX,    2, "two parser-lane entries for x");
    ASSERT_EQ(seenScope0,  1, "outer x at scope 0");
    ASSERT_EQ(seenScope2,  1, "inner x at a nested scope (scope 2)");
}

void testSymbolTableSameScopeRedeclaration() {
    // 'int x; int x;' at the same scope — second one is a redeclaration.
    SymbolTable st;
    Lexer lex("int x; int x;", st);
    auto toks = lex.tokenize();
    Parser p(toks, st);
    (void)p.parse();
    int redeclErrors = 0;
    for (const auto& e : p.getErrors()) {
        if (e.getMessage().find("redeclaration") != string::npos) ++redeclErrors;
    }
    ASSERT_EQ(redeclErrors >= 1, true, "same-scope redeclaration is reported");
}

void testParseErrorMissingSemiInDecl() {
    ASSERT_EQ(parseTuErrorCount("int x") >= 1, true,
              "missing ';' after declaration is reported");
}

void testParseErrorMissingDeclaratorIdentifier() {
    // 'int = 5;' — no identifier after the type specifier.
    ASSERT_EQ(parseTuErrorCount("int = 5;") >= 1, true,
              "missing identifier in declarator is reported");
}

// ---------- Fixture-driven end-to-end tests ----------
// These load the .c files in tests/parser/ and exercise the full
// lexer -> parser pipeline. PARSER_FIXTURES_DIR is set by CMake at build time.

#ifndef PARSER_FIXTURES_DIR
#define PARSER_FIXTURES_DIR "."
#endif

static string loadFixture(const string& filename) {
    string path = string(PARSER_FIXTURES_DIR) + "/" + filename;
    ifstream f(path);
    if (!f) {
        cerr << "FAIL: cannot open fixture: " << path << "\n";
        return "";
    }
    stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Drives lexer + parser on the named fixture; writes counts into outLexErrs /
// outParseErrs / outDeclCount; returns true if the file loaded.
static bool runFixture(const string& filename,
                       int& outLexErrs, int& outParseErrs, int& outDeclCount) {
    string src = loadFixture(filename);
    if (src.empty()) return false;

    SymbolTable st;
    Lexer lex(src, st);
    auto tokens = lex.tokenize();
    outLexErrs = (int)lex.getErrors().size();

    Parser parser(tokens, st);
    auto ast = parser.parse();
    outParseErrs = (int)parser.getErrors().size();
    outDeclCount = ast ? (int)ast->getDecls().size() : 0;
    return true;
}

void testFixtureDeclarationsParsesClean() {
    int lexE, parE, decls;
    bool ok = runFixture("declarations.c", lexE, parE, decls);
    ASSERT_EQ(ok,    true, "declarations.c loads from fixtures dir");
    ASSERT_EQ(lexE,  0,    "declarations.c: zero lexer errors");
    ASSERT_EQ(parE,  0,    "declarations.c: zero parser errors");
    ASSERT_EQ(decls > 0, true, "declarations.c: produces top-level decls");
}

void testFixtureExpressionsParsesClean() {
    int lexE, parE, decls;
    bool ok = runFixture("expressions.c", lexE, parE, decls);
    ASSERT_EQ(ok,    true, "expressions.c loads from fixtures dir");
    ASSERT_EQ(lexE,  0,    "expressions.c: zero lexer errors");
    ASSERT_EQ(parE,  0,    "expressions.c: zero parser errors");
    ASSERT_EQ(decls > 0, true, "expressions.c: produces top-level decls");
}

void testFixtureStatementsParsesClean() {
    int lexE, parE, decls;
    bool ok = runFixture("statements.c", lexE, parE, decls);
    ASSERT_EQ(ok,    true, "statements.c loads from fixtures dir");
    ASSERT_EQ(lexE,  0,    "statements.c: zero lexer errors");
    ASSERT_EQ(parE,  0,    "statements.c: zero parser errors");
    ASSERT_EQ(decls > 0, true, "statements.c: produces top-level decls");
}

void testFixtureErrorsReportsErrors() {
    int lexE, parE, decls;
    bool ok = runFixture("errors.c", lexE, parE, decls);
    ASSERT_EQ(ok,           true, "errors.c loads from fixtures dir");
    ASSERT_EQ(parE >= 1,    true, "errors.c: at least one parse error reported");
    // The fixture is intentionally malformed but the parser must still return
    // a non-null AST root and produce SOMETHING — validating recovery.
    ASSERT_EQ(decls >= 1,   true, "errors.c: parser still produced decls despite errors");
}

// End-to-end: a TranslationUnit holding a FunctionDecl that holds a CompoundStmt
// with a ReturnStmt of a LiteralExpr — exercise composition + indentation.
void testTranslationUnitFunctionDeclEndToEnd() {
    auto retLit = make_unique<LiteralExpr>(2, 12, TokenType::INT_LITERAL, "0");
    auto retStmt = make_unique<ReturnStmt>(2, 5, move(retLit));
    auto body = make_unique<CompoundStmt>(1, 12);
    body->addItem(move(retStmt));

    vector<unique_ptr<ParamDecl>> params;
    auto fn = make_unique<FunctionDecl>(
        1, 5, Type(TypeKind::INT), "main",
        move(params), move(body));

    TranslationUnit tu(1, 1);
    tu.addDecl(move(fn));

    ostringstream oss;
    tu.dump(oss, 0);
    string expected =
        "TranslationUnit\n"
        "  FunctionDecl 'main' returns int\n"
        "    CompoundStmt\n"
        "      ReturnStmt\n"
        "        LiteralExpr INT_LITERAL '0'\n";
    ASSERT_EQ(oss.str(), expected, "translation-unit end-to-end dump");
    ASSERT_EQ((size_t)tu.getDecls().size(), (size_t)1, "translation-unit holds one decl");
}

int main() {
    testPeekDoesNotConsume();
    testPeekWithOffset();
    testGetAdvances();
    testEofAfterAllConsumed();
    testEofSentinelLineColFromLastToken();
    testEofSentinelEmptyStream();
    testGetLineColumnTracking();
    testMatchConsumesOnHit();
    testMatchLeavesCursorOnMiss();
    testMatchAtEofIsFalse();
    testCheckIsPeekOnly();

    testParseErrorGetters();
    testParseErrorToStringWithText();
    testParseErrorToStringWithoutText();
    testParseErrorDefaultOffendingText();
    testParserConstructsWithEmptyStream();
    testParserConstructsWithTokens();

    testTypeDefaults();
    testTypeKindCtor();
    testTypeMutators();
    testTypeToString();
    testTypeKindToString();
    testLiteralExprGettersAndDump();
    testIdentifierExprGettersAndDump();
    testBinaryExprNestedDump();
    testPolymorphicDispatch();
    testUnaryPrefixVsPostfix();
    testLoopStmtWhileVsDoWhile();
    testIfStmtNullableBranches();
    testVarDeclDump();
    testJumpStmtDump();
    testTranslationUnitFunctionDeclEndToEnd();

    testExprPrimaryLiteral();
    testExprPrimaryIdentifier();
    testExprParensNoWrapper();
    testExprBinaryAddition();
    testExprPrecedenceMulOverAdd();
    testExprLeftAssociativity();
    testExprPrefixUnary();
    testExprPostfixIncrement();
    testExprUnaryAndAdditiveCompose();
    testExprAssignmentRightAssoc();
    testExprCompoundAssignment();
    testExprConditional();
    testExprLogicalPrecedence();
    testExprBitwisePrecedence();
    testExprShiftAndAdditive();
    testExprCallEmpty();
    testExprCallWithArgs();
    testExprIndex();
    testExprChainedPostfix();
    testExprErrorOnBareOperator();

    testStmtBareSemicolonReturnsNull();
    testStmtExprStmt();
    testStmtReturnNoValue();
    testStmtReturnWithValue();
    testStmtBreakAndContinue();
    testStmtIfNoElse();
    testStmtIfElse();
    testStmtIfDanglingElse();
    testStmtIfEmptyBody();
    testStmtWhile();
    testStmtDoWhile();
    testStmtFor();
    testStmtForAllEmpty();
    testStmtCompoundEmpty();
    testStmtCompoundMultipleItems();
    testStmtCompoundSkipsStraySemis();
    testStmtNestedCompound();
    testStmtErrorMissingRParen();
    testStmtErrorMissingSemiAfterReturn();

    testParseEmptyTu();
    testParseSingleVarDecl();
    testParseVarDeclWithInit();
    testParseCommaChain();
    testParseCommaChainPointerResetsPerDeclarator();
    testParseConstQualifier();
    testParseStorageClassDiscarded();
    testParseUnsignedDiscarded();
    testParseLongLong();
    testParseFunctionEmptyParams();
    testParseFunctionVoidParams();
    testParseFunctionNamedParams();
    testParseDeclarationInCompound();
    testParseForDeclarationInit();
    testSymbolTableFileScopeVar();
    testSymbolTableFunctionAndParamsAndLocal();
    testSymbolTableShadowingThroughNestedScopes();
    testSymbolTableSameScopeRedeclaration();
    testParseErrorMissingSemiInDecl();
    testParseErrorMissingDeclaratorIdentifier();

    testFixtureDeclarationsParsesClean();
    testFixtureExpressionsParsesClean();
    testFixtureStatementsParsesClean();
    testFixtureErrorsReportsErrors();

    cout << "\nParser Tests: " << passed << " passed, " << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}
