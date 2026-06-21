#include "parser.h"
#include "lexer.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

// ─── Cursor primitives ───────────────────────────────────────────────────────

Token Parser::peek()     { return tokens[current]; }
Token Parser::previous() { return tokens[current - 1]; }
bool  Parser::isAtEnd()  { return peek().type == TokenType::END_OF_FILE; }

bool Parser::check(TokenType t) {
    return !isAtEnd() && peek().type == t;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::match(TokenType t) {
    if (!check(t)) return false;
    advance();
    return true;
}

Token Parser::consume(TokenType t, const std::string& err) {
    if (check(t)) return advance();
    Token bad = peek();
    throw std::runtime_error("[line " + std::to_string(bad.line) + "] Error: " + err
                             + " (got '" + bad.lexeme + "')");
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE)) advance();
}

// ─── Entry point ─────────────────────────────────────────────────────────────

std::unique_ptr<Program> Parser::parse() {
    auto prog = std::make_unique<Program>();
    skipNewlines();
    while (!isAtEnd()) {
        prog->stmts.push_back(statement());
        skipNewlines();
    }
    return prog;
}

// ─── Statements ──────────────────────────────────────────────────────────────

std::unique_ptr<Stmt> Parser::statement() {
    try {
     if (check(TokenType::RUNE))      return varDecl();
     if (check(TokenType::SPELL))     return funcDecl();
     if (check(TokenType::RITUAL))    return ritualDecl();
     if (check(TokenType::WHEN))      return ifStmt();
     if (check(TokenType::WHILE))     return whileStmt();
     if (check(TokenType::FORGE))     return forgeStmt();
     if (check(TokenType::SUMMON))    return importStmt();
     if (check(TokenType::UNLEASH))    return returnStmt();
     return exprStmt();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        // synchronize to the next statement
        while (!isAtEnd() && !check(TokenType::NEWLINE)) advance();
        if (check(TokenType::NEWLINE)) advance();
        return nullptr;
    }

}

// rune x = expr NEWLINE
std::unique_ptr<Stmt> Parser::varDecl() {
    int ln = peek().line;
    consume(TokenType::RUNE, "expected 'rune'");
    Token name = consume(TokenType::IDENTIFIER, "expected variable name");

    std::unique_ptr<Expr> init;
    if (match(TokenType::EQUAL)) {
        init = expr();
    }

    consume(TokenType::NEWLINE, "expected newline after variable declaration");

    auto node = std::make_unique<VarDecl>();
    node->line        = ln;
    node->name        = name.lexeme;
    node->initializer = std::move(init);
    return node;
}

// spell name(params): NEWLINE block
std::unique_ptr<Stmt> Parser::funcDecl() {
    int ln = peek().line;
    consume(TokenType::SPELL, "expected 'spell'");
    Token name = consume(TokenType::IDENTIFIER, "expected function name");
    consume(TokenType::LPAREN, "expected '('");
    auto p = params();
    consume(TokenType::RPAREN, "expected ')'");
    consume(TokenType::COLON, "expected ':'");
    consume(TokenType::NEWLINE, "expected newline after spell declaration");

    auto node = std::make_unique<FuncDecl>();
    node->line   = ln;
    node->name   = name.lexeme;
    node->params = std::move(p);
    node->body   = block();
    return node;
}

// ritual name(params): NEWLINE block
std::unique_ptr<Stmt> Parser::ritualDecl() {
    int ln = peek().line;
    consume(TokenType::RITUAL, "expected 'ritual'");
    Token name = consume(TokenType::IDENTIFIER, "expected ritual name");
    consume(TokenType::LPAREN, "expected '('");
    auto p = params();
    consume(TokenType::RPAREN, "expected ')'");
    consume(TokenType::COLON, "expected ':'");
    consume(TokenType::NEWLINE, "expected newline after ritual declaration");

    auto node = std::make_unique<RitualDecl>();
    node->line   = ln;
    node->name   = name.lexeme;
    node->params = std::move(p);
    node->body   = block();
    return node;
}

// when expr: NEWLINE block (orwhen expr: NEWLINE block)* (otherwise: NEWLINE block)?
std::unique_ptr<Stmt> Parser::ifStmt() {
    int ln = peek().line;
    consume(TokenType::WHEN, "expected 'when'");
    auto cond = expr();
    consume(TokenType::COLON, "expected ':'");
    consume(TokenType::NEWLINE, "expected newline after 'when'");

    auto node = std::make_unique<IfStmt>();
    node->line       = ln;
    node->condition  = std::move(cond);
    node->then_block = block();

    while (check(TokenType::ORWHEN)) {
        advance();
        auto branch_cond = expr();
        consume(TokenType::COLON, "expected ':'");
        consume(TokenType::NEWLINE, "expected newline after 'orwhen'");
        ElseIf branch;
        branch.condition = std::move(branch_cond);
        branch.body      = block();
        node->orwhen_branches.push_back(std::move(branch));
    }

    if (check(TokenType::OTHERWISE)) {
        advance();
        consume(TokenType::COLON, "expected ':'");
        consume(TokenType::NEWLINE, "expected newline after 'otherwise'");
        node->otherwise_block = block();
    }

    return node;
}

// while expr: NEWLINE block
std::unique_ptr<Stmt> Parser::whileStmt() {
    int ln = peek().line;
    consume(TokenType::WHILE, "expected 'while'");
    auto cond = expr();
    consume(TokenType::COLON, "expected ':'");
    consume(TokenType::NEWLINE, "expected newline after 'while'");

    auto node = std::make_unique<WhileStmt>();
    node->line      = ln;
    node->condition = std::move(cond);
    node->body      = block();
    return node;
}

// forge IDENTIFIER in expr: NEWLINE block
std::unique_ptr<Stmt> Parser::forgeStmt() {
    int ln = peek().line;
    consume(TokenType::FORGE, "expected 'forge'");
    Token var = consume(TokenType::IDENTIFIER, "expected loop variable");
    consume(TokenType::IN, "expected 'in'");
    auto list = expr();
    consume(TokenType::COLON, "expected ':'");
    consume(TokenType::NEWLINE, "expected newline after 'forge'");

    auto node  = std::make_unique<ForgeStmt>();
    node->line = ln;
    node->var  = var.lexeme;
    node->list = std::move(list);
    node->body = block();
    return node;
}

// summon IDENTIFIER from IDENTIFIER NEWLINE
std::unique_ptr<Stmt> Parser::importStmt() {
    int ln = peek().line;
    consume(TokenType::SUMMON, "expected 'summon'");
    Token name = consume(TokenType::IDENTIFIER, "expected import name");
    consume(TokenType::FROM, "expected 'from'");
    Token channel = consume(TokenType::IDENTIFIER, "expected channel name");
    consume(TokenType::NEWLINE, "expected newline after import");

    auto node    = std::make_unique<ImportStmt>();
    node->line    = ln;
    node->name    = name.lexeme;
    node->channel = channel.lexeme;
    return node;
}

// return [expr] NEWLINE
std::unique_ptr<Stmt> Parser::returnStmt() {
    int ln = peek().line;
    consume(TokenType::UNLEASH, "expected 'unleash'");

    std::unique_ptr<Expr> value;
    if (!check(TokenType::NEWLINE))
        value = expr();

    consume(TokenType::NEWLINE, "expected newline after return");

    auto node   = std::make_unique<ReturnStmt>();
    node->line  = ln;
    node->value = std::move(value);
    return node;
}

// expr NEWLINE
std::unique_ptr<Stmt> Parser::exprStmt() {
    int ln = peek().line;
    auto e = expr();
    consume(TokenType::NEWLINE, "expected newline after expression");

    auto node  = std::make_unique<ExprStmt>();
    node->line = ln;
    node->expr = std::move(e);
    return node;
}

// INDENT stmt+ DEDENT
std::unique_ptr<Block> Parser::block() {
    consume(TokenType::INDENT, "expected indented block");
    auto node = std::make_unique<Block>();
    skipNewlines();
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        node->stmts.push_back(statement());
        skipNewlines();
    }
    consume(TokenType::DEDENT, "expected dedent");
    return node;
}

// IDENTIFIER (COMMA IDENTIFIER)*
std::vector<std::string> Parser::params() {
    std::vector<std::string> p;
    if (check(TokenType::IDENTIFIER)) {
        p.push_back(advance().lexeme);
        while (match(TokenType::COMMA)) {
            p.push_back(consume(TokenType::IDENTIFIER, "expected parameter name").lexeme);
        }
    }
    return p;
}

// ─── Expressions (precedence ladder) ─────────────────────────────────────────

std::unique_ptr<Expr> Parser::expr() { return orExpr(); }

// or_expr → and_expr (OR and_expr)*
std::unique_ptr<Expr> Parser::orExpr() {
    auto left = andExpr();
    while (check(TokenType::OR)) {
        Token op = advance();
        auto right = andExpr();
        auto node = std::make_unique<BinaryExpr>();
        node->line  = op.line;
        node->op    = op;
        node->left  = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

// and_expr → not_expr (AND not_expr)*
std::unique_ptr<Expr> Parser::andExpr() {
    auto left = notExpr();
    while (check(TokenType::AND)) {
        Token op = advance();
        auto right = notExpr();
        auto node = std::make_unique<BinaryExpr>();
        node->line  = op.line;
        node->op    = op;
        node->left  = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

// not_expr → NOT not_expr | comparison
std::unique_ptr<Expr> Parser::notExpr() {
    if (check(TokenType::NOT)) {
        Token op = advance();
        auto operand = notExpr();
        auto node = std::make_unique<UnaryExpr>();
        node->line    = op.line;
        node->op      = op;
        node->operand = std::move(operand);
        return node;
    }
    return comparison();
}

// comparison → addition ((== | > | <) addition)*
std::unique_ptr<Expr> Parser::comparison() {
    auto left = addition();
    while (check(TokenType::EQUAL_EQUAL)    || check(TokenType::NOT_EQUAL)
        || check(TokenType::GREATER)        || check(TokenType::GREATER_EQUAL)
        || check(TokenType::LESS)           || check(TokenType::LESS_EQUAL)) {
        Token op = advance();
        auto right = addition();
        auto node = std::make_unique<BinaryExpr>();
        node->line  = op.line;
        node->op    = op;
        node->left  = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

// addition → term ((+ | -) term)*
std::unique_ptr<Expr> Parser::addition() {
    auto left = term();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = advance();
        auto right = term();
        auto node = std::make_unique<BinaryExpr>();
        node->line  = op.line;
        node->op    = op;
        node->left  = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

// term → unary ((* | /) unary)*
std::unique_ptr<Expr> Parser::term() {
    auto left = unary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        Token op = advance();
        auto right = unary();
        auto node = std::make_unique<BinaryExpr>();
        node->line  = op.line;
        node->op    = op;
        node->left  = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

// unary → - unary | primary
std::unique_ptr<Expr> Parser::unary() {
    if (check(TokenType::MINUS)) {
        Token op = advance();
        auto operand = unary();
        auto node = std::make_unique<UnaryExpr>();
        node->line    = op.line;
        node->op      = op;
        node->operand = std::move(operand);
        return node;
    }
    return primary();
}

// primary → NUMBER | STRING | blessed | cursed | IDENTIFIER [call] | ( expr )
std::unique_ptr<Expr> Parser::primary() {
    // number literal
    if (check(TokenType::NUMBER)) {
        Token t = advance();
        auto node = std::make_unique<LiteralExpr>();
        node->line  = t.line;
        node->value = t.lexeme;
        node->kind  = TokenType::NUMBER;
        return node;
    }

    // string literal
    if (check(TokenType::STRING)) {
        Token t = advance();
        auto node = std::make_unique<LiteralExpr>();
        node->line  = t.line;
        node->value = t.lexeme;
        node->kind  = TokenType::STRING;
        return node;
    }

    // interpolated string: "Hello {name}"
    if (check(TokenType::INTERP_STRING)) {
        Token t = advance();
        auto node = std::make_unique<InterpolatedStringExpr>();
        node->line = t.line;

        const std::string& content = t.lexeme;
        size_t i = 0;
        std::string raw_buf;

        while (i < content.size()) {
            if (content[i] == '{') {
                if (!raw_buf.empty()) {
                    StringSegment seg;
                    seg.is_expr = false;
                    seg.raw     = std::move(raw_buf);
                    raw_buf.clear();
                    node->segments.push_back(std::move(seg));
                }
                i++; // skip {
                std::string expr_src;
                int depth = 1;
                while (i < content.size() && depth > 0) {
                    if      (content[i] == '{') depth++;
                    else if (content[i] == '}') { if (--depth == 0) break; }
                    expr_src += content[i++];
                }
                if (i < content.size()) i++; // skip }

                Lexer subLexer(expr_src + "\n");
                auto subTokens = subLexer.scanTokens();
                Parser subParser(subTokens);
                StringSegment seg;
                seg.is_expr = true;
                seg.expr    = subParser.expr();
                node->segments.push_back(std::move(seg));
            } else {
                raw_buf += content[i++];
            }
        }
        if (!raw_buf.empty()) {
            StringSegment seg;
            seg.is_expr = false;
            seg.raw     = std::move(raw_buf);
            node->segments.push_back(std::move(seg));
        }
        return node;
    }

    // blessed / cursed
    if (check(TokenType::BLESSED) || check(TokenType::CURSED)) {
        Token t = advance();
        auto node = std::make_unique<LiteralExpr>();
        node->line  = t.line;
        node->value = t.lexeme;
        node->kind  = t.type;
        return node;
    }

    // list literal: [1, 2, 3]
    if (check(TokenType::LBRACKET)) {
        int ln = peek().line;
        advance();
        auto node = std::make_unique<ListExpr>();
        node->line = ln;
        if (!check(TokenType::RBRACKET)) {
            node->elements.push_back(expr());
            while (match(TokenType::COMMA))
                node->elements.push_back(expr());
        }
        consume(TokenType::RBRACKET, "expected ']' after list");
        return node;
    }

    // identifier, call, or index
    if (check(TokenType::IDENTIFIER)) {
        Token name = advance();

        // assignment: x = expr
        if (check(TokenType::EQUAL)) {
            advance();
            auto value = expr();
            auto node = std::make_unique<AssignExpr>();
            node->line  = name.line;
            node->name  = name.lexeme;
            node->value = std::move(value);
            return node;
        }

        std::unique_ptr<Expr> result;

        // call: name(args)
        if (check(TokenType::LPAREN)) {
            advance();
            auto callee = std::make_unique<IdentifierExpr>();
            callee->line = name.line;
            callee->name = name.lexeme;

            auto call = std::make_unique<CallExpr>();
            call->line   = name.line;
            call->callee = std::move(callee);

            if (!check(TokenType::RPAREN)) {
                call->args.push_back(expr());
                while (match(TokenType::COMMA))
                    call->args.push_back(expr());
            }
            consume(TokenType::RPAREN, "expected ')' after arguments");
            result = std::move(call);
        } else {
            // plain identifier
            auto node = std::make_unique<IdentifierExpr>();
            node->line = name.line;
            node->name = name.lexeme;
            result = std::move(node);
        }

        // postfix index: result[expr]
        while (check(TokenType::LBRACKET)) {
            int ln = peek().line;
            advance();
            auto idx = expr();
            consume(TokenType::RBRACKET, "expected ']' after index");
            auto inode = std::make_unique<IndexExpr>();
            inode->line  = ln;
            inode->list  = std::move(result);
            inode->index = std::move(idx);
            result = std::move(inode);
        }

        return result;
    }

    // grouped expression
    if (check(TokenType::LPAREN)) {
        advance();
        auto e = expr();
        consume(TokenType::RPAREN, "expected ')'");
        return e;
    }

    Token bad = peek();
    throw std::runtime_error("[line " + std::to_string(bad.line) + "] Error: unexpected token '"
                             + bad.lexeme + "'");
}
