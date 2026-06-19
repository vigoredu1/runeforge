#pragma once

#include "environment.h"
#include "ast.h"
#include <functional>

// ─── Callables ───────────────────────────────────────────────────────────────

struct RuneCallable {
    virtual ~RuneCallable() = default;
    virtual Value       call(class Interpreter& interp, std::vector<Value> args) = 0;
    virtual int         arity() const = 0;
    virtual std::string toString() const = 0;
};

// native C++ function
struct BuiltinFunction : RuneCallable {
    std::string fname;
    int         farity;
    std::function<Value(std::vector<Value>)> fn;

    Value       call(Interpreter&, std::vector<Value> args) override { return fn(std::move(args)); }
    int         arity() const override   { return farity; }
    std::string toString() const override { return "<builtin " + fname + ">"; }
};

// user-defined spell / ritual
struct RuneFunction : RuneCallable {
    std::string              name_;
    std::vector<std::string> params_;
    Stmt*                    body_;     // non-owning — owned by Program AST
    std::shared_ptr<Environment> closure_;

    RuneFunction(std::string name, std::vector<std::string> params,
                 Stmt* body, std::shared_ptr<Environment> closure)
        : name_(std::move(name)), params_(std::move(params)),
          body_(body), closure_(std::move(closure)) {}

    Value       call(Interpreter& interp, std::vector<Value> args) override;
    int         arity() const override   { return static_cast<int>(params_.size()); }
    std::string toString() const override { return "<spell " + name_ + ">"; }
};

// thrown by return statements, caught by RuneFunction::call
struct ReturnException { Value value; };

// ─── Interpreter ─────────────────────────────────────────────────────────────

class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    Interpreter();
    void  interpret(Program& prog);
    Value evaluate(Expr& expr);
    void  execute(Stmt& stmt);
    void  executeBlock(Block& block, std::shared_ptr<Environment> blockEnv);

    std::shared_ptr<Environment> env;

private:
    Value result_;

    void defineBuiltins();

    void visit(LiteralExpr& e)    override;
    void visit(IdentifierExpr& e) override;
    void visit(BinaryExpr& e)     override;
    void visit(UnaryExpr& e)      override;
    void visit(CallExpr& e)       override;
    void visit(AssignExpr& e)     override;

    void visit(VarDecl& s)    override;
    void visit(FuncDecl& s)   override;
    void visit(RitualDecl& s) override;
    void visit(IfStmt& s)     override;
    void visit(WhileStmt& s)  override;
    void visit(ImportStmt& s) override;
    void visit(ReturnStmt& s) override;
    void visit(ExprStmt& s)   override;
    void visit(Block& s)      override;
    void visit(Program& s)    override;
};
