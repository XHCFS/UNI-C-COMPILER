#include "Ast.h"
using namespace std;

namespace {
    // Two spaces per indent level.
    string makeIndent(int level) { return string(level * 2, ' '); }
}

// ---------------------------------------------------------------------------
// AstNode + abstract intermediates
// ---------------------------------------------------------------------------

AstNode::AstNode(int line, int column) : line_(line), column_(column) {}
int AstNode::getLine()   const { return line_; }
int AstNode::getColumn() const { return column_; }

Expr::Expr(int line, int column) : AstNode(line, column) {}
Stmt::Stmt(int line, int column) : AstNode(line, column) {}
Decl::Decl(int line, int column) : AstNode(line, column) {}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

LiteralExpr::LiteralExpr(int line, int column, TokenType kind, string lexeme)
    : Expr(line, column), kind_(kind), lexeme_(move(lexeme)) {}
TokenType     LiteralExpr::getKind()   const { return kind_; }
const string& LiteralExpr::getLexeme() const { return lexeme_; }
void LiteralExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "LiteralExpr "
       << tokenTypeToString(kind_) << " '" << lexeme_ << "'\n";
}

IdentifierExpr::IdentifierExpr(int line, int column, string name)
    : Expr(line, column), name_(move(name)) {}
const string& IdentifierExpr::getName() const { return name_; }
void IdentifierExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "IdentifierExpr '" << name_ << "'\n";
}

BinaryExpr::BinaryExpr(int line, int column, TokenType op,
                       unique_ptr<Expr> lhs, unique_ptr<Expr> rhs)
    : Expr(line, column), op_(op), lhs_(move(lhs)), rhs_(move(rhs)) {}
TokenType   BinaryExpr::getOp()  const { return op_; }
const Expr* BinaryExpr::getLhs() const { return lhs_.get(); }
const Expr* BinaryExpr::getRhs() const { return rhs_.get(); }
void BinaryExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "BinaryExpr " << tokenTypeToString(op_) << "\n";
    if (lhs_) lhs_->dump(os, indent + 1);
    if (rhs_) rhs_->dump(os, indent + 1);
}

UnaryExpr::UnaryExpr(int line, int column, bool isPostfix,
                     TokenType op, unique_ptr<Expr> operand)
    : Expr(line, column), isPostfix_(isPostfix), op_(op), operand_(move(operand)) {}
bool        UnaryExpr::isPostfix()  const { return isPostfix_; }
TokenType   UnaryExpr::getOp()      const { return op_; }
const Expr* UnaryExpr::getOperand() const { return operand_.get(); }
void UnaryExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "UnaryExpr " << tokenTypeToString(op_)
       << (isPostfix_ ? " postfix" : " prefix") << "\n";
    if (operand_) operand_->dump(os, indent + 1);
}

AssignExpr::AssignExpr(int line, int column, TokenType op,
                       unique_ptr<Expr> target, unique_ptr<Expr> value)
    : Expr(line, column), op_(op), target_(move(target)), value_(move(value)) {}
TokenType   AssignExpr::getOp()     const { return op_; }
const Expr* AssignExpr::getTarget() const { return target_.get(); }
const Expr* AssignExpr::getValue()  const { return value_.get(); }
void AssignExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "AssignExpr " << tokenTypeToString(op_) << "\n";
    if (target_) target_->dump(os, indent + 1);
    if (value_)  value_->dump(os, indent + 1);
}

ConditionalExpr::ConditionalExpr(int line, int column,
                                  unique_ptr<Expr> cond,
                                  unique_ptr<Expr> thenExpr,
                                  unique_ptr<Expr> elseExpr)
    : Expr(line, column), cond_(move(cond)),
      then_(move(thenExpr)), else_(move(elseExpr)) {}
const Expr* ConditionalExpr::getCond() const { return cond_.get(); }
const Expr* ConditionalExpr::getThen() const { return then_.get(); }
const Expr* ConditionalExpr::getElse() const { return else_.get(); }
void ConditionalExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "ConditionalExpr\n";
    if (cond_) cond_->dump(os, indent + 1);
    if (then_) then_->dump(os, indent + 1);
    if (else_) else_->dump(os, indent + 1);
}

CallExpr::CallExpr(int line, int column,
                   unique_ptr<Expr> callee,
                   vector<unique_ptr<Expr>> args)
    : Expr(line, column), callee_(move(callee)), args_(move(args)) {}
const Expr*                     CallExpr::getCallee() const { return callee_.get(); }
const vector<unique_ptr<Expr>>& CallExpr::getArgs()   const { return args_; }
void CallExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "CallExpr\n";
    if (callee_) callee_->dump(os, indent + 1);
    for (const auto& a : args_) if (a) a->dump(os, indent + 1);
}

IndexExpr::IndexExpr(int line, int column,
                     unique_ptr<Expr> array, unique_ptr<Expr> index)
    : Expr(line, column), array_(move(array)), index_(move(index)) {}
const Expr* IndexExpr::getArray() const { return array_.get(); }
const Expr* IndexExpr::getIndex() const { return index_.get(); }
void IndexExpr::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "IndexExpr\n";
    if (array_) array_->dump(os, indent + 1);
    if (index_) index_->dump(os, indent + 1);
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

CompoundStmt::CompoundStmt(int line, int column) : Stmt(line, column) {}
void CompoundStmt::addItem(unique_ptr<AstNode> item) { items_.push_back(move(item)); }
const vector<unique_ptr<AstNode>>& CompoundStmt::getItems() const { return items_; }
void CompoundStmt::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "CompoundStmt\n";
    for (const auto& item : items_) if (item) item->dump(os, indent + 1);
}

IfStmt::IfStmt(int line, int column,
               unique_ptr<Expr> cond,
               unique_ptr<Stmt> thenBranch,
               unique_ptr<Stmt> elseBranch)
    : Stmt(line, column), cond_(move(cond)),
      then_(move(thenBranch)), else_(move(elseBranch)) {}
const Expr* IfStmt::getCond() const { return cond_.get(); }
const Stmt* IfStmt::getThen() const { return then_.get(); }
const Stmt* IfStmt::getElse() const { return else_.get(); }
void IfStmt::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "IfStmt\n";
    if (cond_) cond_->dump(os, indent + 1);
    if (then_) then_->dump(os, indent + 1);
    if (else_) else_->dump(os, indent + 1);
}

LoopStmt::LoopStmt(int line, int column, bool isPostTest,
                   unique_ptr<Expr> cond, unique_ptr<Stmt> body)
    : Stmt(line, column), isPostTest_(isPostTest),
      cond_(move(cond)), body_(move(body)) {}
bool        LoopStmt::isPostTest() const { return isPostTest_; }
const Expr* LoopStmt::getCond()    const { return cond_.get(); }
const Stmt* LoopStmt::getBody()    const { return body_.get(); }
void LoopStmt::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "LoopStmt "
       << (isPostTest_ ? "do-while" : "while") << "\n";
    if (cond_) cond_->dump(os, indent + 1);
    if (body_) body_->dump(os, indent + 1);
}

ForStmt::ForStmt(int line, int column,
                 unique_ptr<AstNode> init,
                 unique_ptr<Expr> cond,
                 unique_ptr<Expr> step,
                 unique_ptr<Stmt> body)
    : Stmt(line, column), init_(move(init)), cond_(move(cond)),
      step_(move(step)), body_(move(body)) {}
const AstNode* ForStmt::getInit() const { return init_.get(); }
const Expr*    ForStmt::getCond() const { return cond_.get(); }
const Expr*    ForStmt::getStep() const { return step_.get(); }
const Stmt*    ForStmt::getBody() const { return body_.get(); }
void ForStmt::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "ForStmt\n";
    if (init_) init_->dump(os, indent + 1);
    if (cond_) cond_->dump(os, indent + 1);
    if (step_) step_->dump(os, indent + 1);
    if (body_) body_->dump(os, indent + 1);
}

ReturnStmt::ReturnStmt(int line, int column, unique_ptr<Expr> value)
    : Stmt(line, column), value_(move(value)) {}
const Expr* ReturnStmt::getValue() const { return value_.get(); }
void ReturnStmt::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "ReturnStmt\n";
    if (value_) value_->dump(os, indent + 1);
}

JumpStmt::JumpStmt(int line, int column, TokenType kind)
    : Stmt(line, column), kind_(kind) {}
TokenType JumpStmt::getKind() const { return kind_; }
void JumpStmt::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "JumpStmt " << tokenTypeToString(kind_) << "\n";
}

ExprStmt::ExprStmt(int line, int column, unique_ptr<Expr> expr)
    : Stmt(line, column), expr_(move(expr)) {}
const Expr* ExprStmt::getExpr() const { return expr_.get(); }
void ExprStmt::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "ExprStmt\n";
    if (expr_) expr_->dump(os, indent + 1);
}

// ---------------------------------------------------------------------------
// Declarations
// ---------------------------------------------------------------------------

VarDecl::VarDecl(int line, int column, Type type, string name, unique_ptr<Expr> init)
    : Decl(line, column), type_(move(type)),
      name_(move(name)), init_(move(init)) {}
const Type&   VarDecl::getType() const { return type_; }
const string& VarDecl::getName() const { return name_; }
const Expr*   VarDecl::getInit() const { return init_.get(); }
void VarDecl::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "VarDecl '" << name_ << "' "
       << type_.toString() << "\n";
    if (init_) init_->dump(os, indent + 1);
}

ParamDecl::ParamDecl(int line, int column, Type type, string name)
    : Decl(line, column), type_(move(type)), name_(move(name)) {}
const Type&   ParamDecl::getType() const { return type_; }
const string& ParamDecl::getName() const { return name_; }
void ParamDecl::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "ParamDecl '" << name_ << "' "
       << type_.toString() << "\n";
}

FunctionDecl::FunctionDecl(int line, int column,
                            Type returnType, string name,
                            vector<unique_ptr<ParamDecl>> params,
                            unique_ptr<CompoundStmt> body)
    : Decl(line, column), returnType_(move(returnType)), name_(move(name)),
      params_(move(params)), body_(move(body)) {}
const Type&                          FunctionDecl::getReturnType() const { return returnType_; }
const string&                        FunctionDecl::getName()       const { return name_; }
const vector<unique_ptr<ParamDecl>>& FunctionDecl::getParams()     const { return params_; }
const CompoundStmt*                  FunctionDecl::getBody()       const { return body_.get(); }
void FunctionDecl::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "FunctionDecl '" << name_ << "' returns "
       << returnType_.toString() << "\n";
    for (const auto& p : params_) if (p) p->dump(os, indent + 1);
    if (body_) body_->dump(os, indent + 1);
}

// ---------------------------------------------------------------------------
// Top-level
// ---------------------------------------------------------------------------

TranslationUnit::TranslationUnit(int line, int column) : AstNode(line, column) {}
void TranslationUnit::addDecl(unique_ptr<Decl> decl) { decls_.push_back(move(decl)); }
const vector<unique_ptr<Decl>>& TranslationUnit::getDecls() const { return decls_; }
void TranslationUnit::dump(ostream& os, int indent) const {
    os << makeIndent(indent) << "TranslationUnit\n";
    for (const auto& d : decls_) if (d) d->dump(os, indent + 1);
}
