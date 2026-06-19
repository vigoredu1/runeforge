#pragma once

#include <string>
#include <vector>
#include <memory>
#include "token.h"

// ─── Forward declarations ────────────────────────────────────────────────────

struct LiteralExpr;
struct IdentifierExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct AssignExpr;

struct VarDecl;
struct FuncDecl;
struct RitualDecl;
struct IfStmt;
struct WhileStmt;
struct ImportStmt;
struct ReturnStmt;
struct ExprStmt;
struct Block;
struct Program;

// ─── Visitor interfaces ──────────────────────────────────────────────────────

struct ExprVisitor {
    virtual ~ExprVisitor() = default;
    virtual void visit(LiteralExpr&)    = 0;
    virtual void visit(IdentifierExpr&) = 0;
    virtual void visit(BinaryExpr&)     = 0;
    virtual void visit(UnaryExpr&)      = 0;
    virtual void visit(CallExpr&)       = 0;
    virtual void visit(AssignExpr&)     = 0;
};

struct StmtVisitor {
    virtual ~StmtVisitor() = default;
    virtual void visit(VarDecl&)    = 0;
    virtual void visit(FuncDecl&)   = 0;
    virtual void visit(RitualDecl&) = 0;
    virtual void visit(IfStmt&)     = 0;
    virtual void visit(WhileStmt&)  = 0;
    virtual void visit(ImportStmt&) = 0;
    virtual void visit(ReturnStmt&) = 0;
    virtual void visit(ExprStmt&)   = 0;
    virtual void visit(Block&)      = 0;
    virtual void visit(Program&)    = 0;
};

// ─── Base nodes ──────────────────────────────────────────────────────────────

struct Expr {
    int line = 0;
    virtual ~Expr() = default;
    virtual void accept(ExprVisitor& v) = 0;
};

struct Stmt {
    int line = 0;
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& v) = 0;
};

// ─── Expressions ─────────────────────────────────────────────────────────────

// number, string, blessed, cursed
struct LiteralExpr : Expr {
    std::string value;
    TokenType   kind;   // NUMBER, STRING, BLESSED, CURSED
    void accept(ExprVisitor& v) override { v.visit(*this); }
};

// variable reference: x
struct IdentifierExpr : Expr {
    std::string name;
    void accept(ExprVisitor& v) override { v.visit(*this); }
};

// left OP right  (+, -, *, /, ==, >, <, and, or)
struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    Token                 op;
    std::unique_ptr<Expr> right;
    void accept(ExprVisitor& v) override { v.visit(*this); }
};

// OP operand  (not, -)
struct UnaryExpr : Expr {
    Token                 op;
    std::unique_ptr<Expr> operand;
    void accept(ExprVisitor& v) override { v.visit(*this); }
};

// callee(arg, arg, ...)
struct CallExpr : Expr {
    std::unique_ptr<Expr>              callee;
    std::vector<std::unique_ptr<Expr>> args;
    void accept(ExprVisitor& v) override { v.visit(*this); }
};

// x = expr  (re-assignment, not declaration)
struct AssignExpr : Expr {
    std::string           name;
    std::unique_ptr<Expr> value;
    void accept(ExprVisitor& v) override { v.visit(*this); }
};

// ─── Statements ──────────────────────────────────────────────────────────────

// rune x = expr
struct VarDecl : Stmt {
    std::string           name;
    std::unique_ptr<Expr> initializer;  // may be null
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// spell name(params): block
struct FuncDecl : Stmt {
    std::string              name;
    std::vector<std::string> params;
    std::unique_ptr<Stmt>    body;   // always a Block
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// ritual name(): block  (entry point)
struct RitualDecl : Stmt {
    std::string              name;
    std::vector<std::string> params;
    std::unique_ptr<Stmt>    body;
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

struct ElseIf {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

// when cond: block  orwhen cond: block  otherwise: block
struct IfStmt : Stmt {
    std::unique_ptr<Expr>    condition;
    std::unique_ptr<Stmt>    then_block;
    std::vector<ElseIf>      orwhen_branches;
    std::unique_ptr<Stmt>    otherwise_block;  // may be null
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// while cond: block
struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// summon name from channel
struct ImportStmt : Stmt {
    std::string name;
    std::string channel;
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// return expr
struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;  // may be null
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// expression used as statement: print("hi")
struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// indented block of statements
struct Block : Stmt {
    std::vector<std::unique_ptr<Stmt>> stmts;
    void accept(StmtVisitor& v) override { v.visit(*this); }
};

// top-level program
struct Program : Stmt {
    std::vector<std::unique_ptr<Stmt>> stmts;
    void accept(StmtVisitor& v) override { v.visit(*this); }
};
