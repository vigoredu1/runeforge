#include "interpreter.h"
#include <iostream>
#include <stdexcept>

// ─── Value helpers ────────────────────────────────────────────────────────────

std::string valueToString(const Value& v) {
    if (std::holds_alternative<std::monostate>(v))                    return "null";
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (d == static_cast<long long>(d)) return std::to_string(static_cast<long long>(d));
        return std::to_string(d);
    }
    if (std::holds_alternative<bool>(v))   return std::get<bool>(v) ? "blessed" : "cursed";
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<std::shared_ptr<RuneCallable>>(v))
        return std::get<std::shared_ptr<RuneCallable>>(v)->toString();
    if (std::holds_alternative<std::shared_ptr<RuneList>>(v)) {
        auto& list = std::get<std::shared_ptr<RuneList>>(v)->items;
        std::string s = "[";
        for (size_t i = 0; i < list.size(); i++) {
            if (i) s += ", ";
            s += valueToString(list[i]);
        }
        return s + "]";
    }
    return "null";
}

bool isTruthy(const Value& v) {
    if (std::holds_alternative<std::monostate>(v)) return false;
    if (std::holds_alternative<bool>(v))           return std::get<bool>(v);
    if (std::holds_alternative<double>(v))         return std::get<double>(v) != 0.0;
    if (std::holds_alternative<std::string>(v))    return !std::get<std::string>(v).empty();
    return true;
}

// ─── Environment ─────────────────────────────────────────────────────────────

Environment::Environment(std::shared_ptr<Environment> parent) : parent(std::move(parent)) {}

void Environment::define(const std::string& name, const Value& val) {
    values[name] = val;
}

Value Environment::get(const std::string& name) const {
    auto it = values.find(name);
    if (it != values.end()) return it->second;
    if (parent) return parent->get(name);
    throw std::runtime_error("Undefined variable '" + name + "'");
}

void Environment::assign(const std::string& name, const Value& val) {
    auto it = values.find(name);
    if (it != values.end()) { it->second = val; return; }
    if (parent) { parent->assign(name, val); return; }
    throw std::runtime_error("Undefined variable '" + name + "'");
}

// ─── RuneFunction::call ───────────────────────────────────────────────────────

Value RuneFunction::call(Interpreter& interp, std::vector<Value> args) {
    auto callEnv = std::make_shared<Environment>(closure_);
    for (int i = 0; i < static_cast<int>(params_.size()); i++) {
        Value arg = i < static_cast<int>(args.size()) ? args[i] : Value{std::monostate{}};
        callEnv->define(params_[i], arg);
    }
    try {
        interp.executeBlock(static_cast<Block&>(*body_), callEnv);
    } catch (ReturnException& ret) {
        return ret.value;
    }
    return std::monostate{};
}

// ─── Interpreter setup ────────────────────────────────────────────────────────

Interpreter::Interpreter() {
    env = std::make_shared<Environment>();
    defineBuiltins();
}

void Interpreter::defineBuiltins() {
    auto make_builtin = [&](std::string name, int arity,
                            std::function<Value(std::vector<Value>)> fn) {
        auto b    = std::make_shared<BuiltinFunction>();
        b->fname  = name;
        b->farity = arity;
        b->fn     = std::move(fn);
        env->define(name, b);
    };

    make_builtin("cast", 1, [](std::vector<Value> args) -> Value {
        std::cout << valueToString(args[0]) << "\n";
        return std::monostate{};
    });

    make_builtin("str", 1, [](std::vector<Value> args) -> Value {
        return valueToString(args[0]);
    });

    make_builtin("num", 1, [](std::vector<Value> args) -> Value {
        try { return std::stod(std::get<std::string>(args[0])); }
        catch (...) { return 0.0; }
    });

     make_builtin("len", 1, [](std::vector<Value> args) -> Value {
      auto& rl = std::get<std::shared_ptr<RuneList>>(args[0]);
      return static_cast<double>(rl->items.size());
  });
   make_builtin("append", 2, [](std::vector<Value> args) -> Value {
      auto& rl = std::get<std::shared_ptr<RuneList>>(args[0]);
      rl->items.push_back(args[1]);
      return std::monostate{};
  });
    make_builtin("scribe", 0, [](std::vector<Value>) -> Value {
        std::string line;
        std::getline(std::cin, line);
        return line;
    });

   make_builtin("pop", 1, [](std::vector<Value> args) -> Value {
      auto& rl = std::get<std::shared_ptr<RuneList>>(args[0]);
      if (rl->items.empty())
          throw std::runtime_error("pop from empty list");
      Value last = rl->items.back();
      rl->items.pop_back();
      return last;
  });
}

// ─── Core execution ───────────────────────────────────────────────────────────

void Interpreter::interpret(Program& prog) { visit(prog); }

Value Interpreter::evaluate(Expr& expr) {
    expr.accept(*this);
    return result_;
}

void Interpreter::execute(Stmt& stmt) {
    stmt.accept(*this);
}

void Interpreter::executeBlock(Block& block, std::shared_ptr<Environment> blockEnv) {
    auto prev = env;
    env = blockEnv;
    try {
        for (auto& stmt : block.stmts) execute(*stmt);
    } catch (...) {
        env = prev;
        throw;
    }
    env = prev;
}

// ─── Expression visitors ──────────────────────────────────────────────────────

void Interpreter::visit(LiteralExpr& e) {
    switch (e.kind) {
        case TokenType::NUMBER:  result_ = std::stod(e.value); break;
        case TokenType::STRING:  result_ = e.value;            break;
        case TokenType::BLESSED: result_ = true;               break;
        case TokenType::CURSED:  result_ = false;              break;
        default:                 result_ = std::monostate{};
    }
}

void Interpreter::visit(IdentifierExpr& e) {
    result_ = env->get(e.name);
}

void Interpreter::visit(BinaryExpr& e) {
    Value left  = evaluate(*e.left);
    Value right = evaluate(*e.right);
    TokenType op = e.op.type;

    if (op == TokenType::AND) { result_ = isTruthy(left) && isTruthy(right); return; }
    if (op == TokenType::OR)  { result_ = isTruthy(left) || isTruthy(right); return; }

    // string concat
    if (op == TokenType::PLUS
        && std::holds_alternative<std::string>(left)
        && std::holds_alternative<std::string>(right)) {
        result_ = std::get<std::string>(left) + std::get<std::string>(right);
        return;
    }

    // numeric ops
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
        double l = std::get<double>(left);
        double r = std::get<double>(right);
        switch (op) {
            case TokenType::PLUS:        result_ = l + r;  return;
            case TokenType::MINUS:       result_ = l - r;  return;
            case TokenType::STAR:        result_ = l * r;  return;
            case TokenType::SLASH:
                if (r == 0.0) throw std::runtime_error("[line " + std::to_string(e.line) + "] Division by zero");
                result_ = l / r; return;
            case TokenType::GREATER:       result_ = l > r;  return;
            case TokenType::GREATER_EQUAL: result_ = l >= r; return;
            case TokenType::LESS:          result_ = l < r;  return;
            case TokenType::LESS_EQUAL:    result_ = l <= r; return;
            case TokenType::EQUAL_EQUAL:   result_ = l == r; return;
            case TokenType::NOT_EQUAL:     result_ = l != r; return;
            default: break;
        }
    }

    if (op == TokenType::EQUAL_EQUAL) { result_ = (left == right); return; }

    throw std::runtime_error("[line " + std::to_string(e.line)
        + "] Invalid operands for '" + e.op.lexeme + "'");
}

void Interpreter::visit(UnaryExpr& e) {
    Value val = evaluate(*e.operand);
    if (e.op.type == TokenType::MINUS) {
        if (!std::holds_alternative<double>(val))
            throw std::runtime_error("[line " + std::to_string(e.line) + "] Operand must be a number");
        result_ = -std::get<double>(val);
    } else if (e.op.type == TokenType::NOT) {
        result_ = !isTruthy(val);
    }
}

void Interpreter::visit(CallExpr& e) {
    Value callee = evaluate(*e.callee);
    if (!std::holds_alternative<std::shared_ptr<RuneCallable>>(callee))
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Value is not callable");

    auto fn = std::get<std::shared_ptr<RuneCallable>>(callee);

    std::vector<Value> args;
    for (auto& arg : e.args) args.push_back(evaluate(*arg));

    if (static_cast<int>(args.size()) != fn->arity())
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Expected "
            + std::to_string(fn->arity()) + " args, got " + std::to_string(args.size()));

    result_ = fn->call(*this, std::move(args));
}

void Interpreter::visit(AssignExpr& e) {
    Value val = evaluate(*e.value);
    env->assign(e.name, val);
    result_ = val;
}

void Interpreter::visit(ListExpr& e) {
    auto rl = std::make_shared<RuneList>();
    for (auto& elem : e.elements)
        rl->items.push_back(evaluate(*elem));
    result_ = rl;
}

void Interpreter::visit(IndexExpr& e) {
    Value obj = evaluate(*e.list);
    Value idx = evaluate(*e.index);

    if (!std::holds_alternative<std::shared_ptr<RuneList>>(obj))
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Value is not a list");
    if (!std::holds_alternative<double>(idx))
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Index must be a number");

    auto& items = std::get<std::shared_ptr<RuneList>>(obj)->items;
    int i = static_cast<int>(std::get<double>(idx));
    if (i < 0 || i >= static_cast<int>(items.size()))
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Index out of range");
    result_ = items[i];
}

void Interpreter::visit(InterpolatedStringExpr& e) {
    std::string result;
    for (auto& seg : e.segments) {
        if (!seg.is_expr)
            result += seg.raw;
        else
            result += valueToString(evaluate(*seg.expr));
    }
    result_ = result;
}

// ─── Statement visitors ───────────────────────────────────────────────────────

void Interpreter::visit(VarDecl& s) {
    Value val = s.initializer ? evaluate(*s.initializer) : Value{std::monostate{}};
    env->define(s.name, val);
}

void Interpreter::visit(FuncDecl& s) {
    auto fn = std::make_shared<RuneFunction>(s.name, s.params, s.body.get(), env);
    env->define(s.name, fn);
}

void Interpreter::visit(RitualDecl& s) {
    auto fn = std::make_shared<RuneFunction>(s.name, s.params, s.body.get(), env);
    env->define(s.name, fn);
}

void Interpreter::visit(IfStmt& s) {
    if (isTruthy(evaluate(*s.condition))) {
        execute(*s.then_block);
        return;
    }
    for (auto& branch : s.orwhen_branches) {
        if (isTruthy(evaluate(*branch.condition))) {
            execute(*branch.body);
            return;
        }
    }
    if (s.otherwise_block) execute(*s.otherwise_block);
}

void Interpreter::visit(WhileStmt& s) {
    while (isTruthy(evaluate(*s.condition))) {
        execute(*s.body);
    }
}

void Interpreter::visit(ForgeStmt& s) {
    Value listVal = evaluate(*s.list);
    if (!std::holds_alternative<std::shared_ptr<RuneList>>(listVal))
        throw std::runtime_error("[line " + std::to_string(s.line) + "] 'forge' requires a list");

    auto& items = std::get<std::shared_ptr<RuneList>>(listVal)->items;
    for (const Value& item : items) {
        auto loopEnv = std::make_shared<Environment>(env);
        loopEnv->define(s.var, item);
        executeBlock(static_cast<Block&>(*s.body), loopEnv);
    }
}

void Interpreter::visit(ImportStmt& s) {
    (void)s; // channels stubbed — implement per-channel later
}

void Interpreter::visit(ReturnStmt& s) {
    Value val = s.value ? evaluate(*s.value) : Value{std::monostate{}};
    throw ReturnException{val};
}

void Interpreter::visit(ExprStmt& s) {
    evaluate(*s.expr);
}

void Interpreter::visit(Block& s) {
    auto blockEnv = std::make_shared<Environment>(env);
    executeBlock(s, blockEnv);
}

void Interpreter::visit(Program& s) {
    for (auto& stmt : s.stmts) execute(*stmt);

    // auto-call ritual main() if defined
    try {
        Value main = env->get("main");
        if (std::holds_alternative<std::shared_ptr<RuneCallable>>(main))
            std::get<std::shared_ptr<RuneCallable>>(main)->call(*this, {});
    } catch (std::runtime_error&) {
        // no main ritual defined — fine
    }
}
