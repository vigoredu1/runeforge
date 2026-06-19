#pragma once

#include <vector>
#include <memory>
#include <string>
#include "token.h"
#include "ast.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> tokens;
    int current = 0;

    // cursor
    Token peek();
    Token previous();
    Token advance();
    bool  isAtEnd();
    bool  check(TokenType t);
    bool  match(TokenType t);
    Token consume(TokenType t, const std::string& err);
    void  skipNewlines();

    // statements
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> varDecl();
    std::unique_ptr<Stmt> funcDecl();
    std::unique_ptr<Stmt> ritualDecl();
    std::unique_ptr<Stmt> ifStmt();
    std::unique_ptr<Stmt> whileStmt();
    std::unique_ptr<Stmt> importStmt();
    std::unique_ptr<Stmt> returnStmt();
    std::unique_ptr<Stmt> exprStmt();
    std::unique_ptr<Block> block();
    std::vector<std::string> params();

    // expressions (precedence ladder, low → high)
    std::unique_ptr<Expr> expr();
    std::unique_ptr<Expr> orExpr();
    std::unique_ptr<Expr> andExpr();
    std::unique_ptr<Expr> notExpr();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> addition();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> primary();
};
