#pragma once
#include "Token.h"
#include "Type.h"
#include <memory>
#include <vector>
#include <string>
#include <ostream>
using namespace std;


// Base class for every AST node. Carries source location and exposes a single
// dump() so any pretty-printer can render the tree polymorphically.
// Plays the role of Token's toString() for AST nodes.
class AstNode {
public:
    AstNode(int line, int column);
    virtual ~AstNode() = default;
    int  getLine()   const;
    int  getColumn() const;
    // Each concrete class writes its own header line at the supplied indent
    // and recurses on its children with indent + 1.
    virtual void dump(ostream& os, int indent) const = 0;
protected:
    int line_;
    int column_;
};

// Abstract intermediates — they introduce no extra fields, only type tags
// reflecting the C grammar.
class Expr : public AstNode { public: Expr(int line, int column); };
class Stmt : public AstNode { public: Stmt(int line, int column); };
class Decl : public AstNode { public: Decl(int line, int column); };

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

// Any of the four literal forms; kind_ is INT_LITERAL / FLOAT_LITERAL /
// CHAR_LITERAL / STRING_LITERAL. lexeme_ is the exact source text.
class LiteralExpr : public Expr {
public:
    LiteralExpr(int line, int column, TokenType kind, string lexeme);
    TokenType     getKind()   const;
    const string& getLexeme() const;
    void dump(ostream& os, int indent) const override;
private:
    TokenType kind_;
    string    lexeme_;
};

// An identifier reference (declarations live in Decl subclasses).
class IdentifierExpr : public Expr {
public:
    IdentifierExpr(int line, int column, string name);
    const string& getName() const;
    void dump(ostream& os, int indent) const override;
private:
    string name_;
};

// Any binary operator from PLUS .. LOGICAL_OR.
class BinaryExpr : public Expr {
public:
    BinaryExpr(int line, int column, TokenType op,
               unique_ptr<Expr> lhs, unique_ptr<Expr> rhs);
    TokenType   getOp()  const;
    const Expr* getLhs() const;
    const Expr* getRhs() const;
    void dump(ostream& os, int indent) const override;
private:
    TokenType        op_;
    unique_ptr<Expr> lhs_;
    unique_ptr<Expr> rhs_;
};

// Both prefix and postfix unary forms. Prefix: + - ! ~ ++ -- * &.
// Postfix: ++ -- only. Distinguished by isPostfix_.
class UnaryExpr : public Expr {
public:
    UnaryExpr(int line, int column, bool isPostfix,
              TokenType op, unique_ptr<Expr> operand);
    bool        isPostfix()  const;
    TokenType   getOp()      const;
    const Expr* getOperand() const;
    void dump(ostream& os, int indent) const override;
private:
    bool             isPostfix_;
    TokenType        op_;
    unique_ptr<Expr> operand_;
};

// Any assignment from ASSIGN .. RSHIFT_ASSIGN. Right-associative.
class AssignExpr : public Expr {
public:
    AssignExpr(int line, int column, TokenType op,
               unique_ptr<Expr> target, unique_ptr<Expr> value);
    TokenType   getOp()     const;
    const Expr* getTarget() const;
    const Expr* getValue()  const;
    void dump(ostream& os, int indent) const override;
private:
    TokenType        op_;
    unique_ptr<Expr> target_;
    unique_ptr<Expr> value_;
};

// Ternary 'cond ? then : else' — right-associative.
class ConditionalExpr : public Expr {
public:
    ConditionalExpr(int line, int column,
                    unique_ptr<Expr> cond,
                    unique_ptr<Expr> thenExpr,
                    unique_ptr<Expr> elseExpr);
    const Expr* getCond() const;
    const Expr* getThen() const;
    const Expr* getElse() const;
    void dump(ostream& os, int indent) const override;
private:
    unique_ptr<Expr> cond_;
    unique_ptr<Expr> then_;
    unique_ptr<Expr> else_;
};

// Function call: callee(arg0, arg1, ...).
class CallExpr : public Expr {
public:
    CallExpr(int line, int column,
             unique_ptr<Expr> callee,
             vector<unique_ptr<Expr>> args);
    const Expr*                       getCallee() const;
    const vector<unique_ptr<Expr>>&   getArgs()   const;
    void dump(ostream& os, int indent) const override;
private:
    unique_ptr<Expr>         callee_;
    vector<unique_ptr<Expr>> args_;
};

// Subscript: array[index].
class IndexExpr : public Expr {
public:
    IndexExpr(int line, int column,
              unique_ptr<Expr> array,
              unique_ptr<Expr> index);
    const Expr* getArray() const;
    const Expr* getIndex() const;
    void dump(ostream& os, int indent) const override;
private:
    unique_ptr<Expr> array_;
    unique_ptr<Expr> index_;
};

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

// Brace-delimited block. items_ may interleave Decl and Stmt nodes; stray ';'
// tokens are not represented (parseStatement returns nullptr for them).
class CompoundStmt : public Stmt {
public:
    CompoundStmt(int line, int column);
    void addItem(unique_ptr<AstNode> item);
    const vector<unique_ptr<AstNode>>& getItems() const;
    void dump(ostream& os, int indent) const override;
private:
    vector<unique_ptr<AstNode>> items_;
};

// 'if (cond) then [else else]'. else_ is null when absent. then_ and else_
// can also be null when the body is a bare ';' (no null-statement node).
class IfStmt : public Stmt {
public:
    IfStmt(int line, int column,
           unique_ptr<Expr> cond,
           unique_ptr<Stmt> thenBranch,
           unique_ptr<Stmt> elseBranch);
    const Expr* getCond() const;
    const Stmt* getThen() const;
    const Stmt* getElse() const;
    void dump(ostream& os, int indent) const override;
private:
    unique_ptr<Expr> cond_;
    unique_ptr<Stmt> then_;
    unique_ptr<Stmt> else_;
};

// 'while (cond) body' (isPostTest_=false) or 'do body while (cond);' (isPostTest_=true).
// body_ may be null for empty body.
class LoopStmt : public Stmt {
public:
    LoopStmt(int line, int column, bool isPostTest,
             unique_ptr<Expr> cond, unique_ptr<Stmt> body);
    bool        isPostTest() const;
    const Expr* getCond()    const;
    const Stmt* getBody()    const;
    void dump(ostream& os, int indent) const override;
private:
    bool             isPostTest_;
    unique_ptr<Expr> cond_;
    unique_ptr<Stmt> body_;
};

// 'for (init; cond; step) body'. Any of init/cond/step/body may be null.
// init_ is an AstNode so it can be either an expression statement or a declaration.
class ForStmt : public Stmt {
public:
    ForStmt(int line, int column,
            unique_ptr<AstNode> init,
            unique_ptr<Expr> cond,
            unique_ptr<Expr> step,
            unique_ptr<Stmt> body);
    const AstNode* getInit() const;
    const Expr*    getCond() const;
    const Expr*    getStep() const;
    const Stmt*    getBody() const;
    void dump(ostream& os, int indent) const override;
private:
    unique_ptr<AstNode> init_;
    unique_ptr<Expr>    cond_;
    unique_ptr<Expr>    step_;
    unique_ptr<Stmt>    body_;
};

// 'return [value];'. value_ is null for bare 'return;'.
class ReturnStmt : public Stmt {
public:
    ReturnStmt(int line, int column, unique_ptr<Expr> value);
    const Expr* getValue() const;
    void dump(ostream& os, int indent) const override;
private:
    unique_ptr<Expr> value_;
};

// 'break;' (kind_ = KW_BREAK) or 'continue;' (kind_ = KW_CONTINUE).
class JumpStmt : public Stmt {
public:
    JumpStmt(int line, int column, TokenType kind);
    TokenType getKind() const;
    void dump(ostream& os, int indent) const override;
private:
    TokenType kind_;
};

// 'expr;'.
class ExprStmt : public Stmt {
public:
    ExprStmt(int line, int column, unique_ptr<Expr> expr);
    const Expr* getExpr() const;
    void dump(ostream& os, int indent) const override;
private:
    unique_ptr<Expr> expr_;
};

// ---------------------------------------------------------------------------
// Declarations
// ---------------------------------------------------------------------------

// Single variable declaration; init_ is null when no initializer is present.
class VarDecl : public Decl {
public:
    VarDecl(int line, int column, Type type, string name, unique_ptr<Expr> init);
    const Type&   getType() const;
    const string& getName() const;
    const Expr*   getInit() const;
    void dump(ostream& os, int indent) const override;
private:
    Type             type_;
    string           name_;
    unique_ptr<Expr> init_;
};

// One parameter in a function signature. name_ may be empty for unnamed params.
class ParamDecl : public Decl {
public:
    ParamDecl(int line, int column, Type type, string name);
    const Type&   getType() const;
    const string& getName() const;
    void dump(ostream& os, int indent) const override;
private:
    Type   type_;
    string name_;
};

// Function definition. body_ is null for prototypes (deferred to v2).
class FunctionDecl : public Decl {
public:
    FunctionDecl(int line, int column,
                 Type returnType, string name,
                 vector<unique_ptr<ParamDecl>> params,
                 unique_ptr<CompoundStmt> body);
    const Type&                                getReturnType() const;
    const string&                              getName()       const;
    const vector<unique_ptr<ParamDecl>>&       getParams()     const;
    const CompoundStmt*                        getBody()       const;
    void dump(ostream& os, int indent) const override;
private:
    Type                          returnType_;
    string                        name_;
    vector<unique_ptr<ParamDecl>> params_;
    unique_ptr<CompoundStmt>      body_;
};

// ---------------------------------------------------------------------------
// Top-level
// ---------------------------------------------------------------------------

// Root of the AST. Holds one Decl per external declaration in source order.
class TranslationUnit : public AstNode {
public:
    TranslationUnit(int line, int column);
    void addDecl(unique_ptr<Decl> decl);
    const vector<unique_ptr<Decl>>& getDecls() const;
    void dump(ostream& os, int indent) const override;
private:
    vector<unique_ptr<Decl>> decls_;
};
