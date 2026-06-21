#pragma once

#include <string>
#include <vector>
#include "token.h"

class Lexer {
public:
    Lexer(std::string source);
    std::vector<Token> scanTokens();

private:
    std::string source;
    std::vector<Token> tokens;

    int start = 0;      // start of current token being scanned
    int current = 0;     // current character being looked at
    int line = 1;        // current line number, for error messages
    bool hadError = false;

    // indentation tracking
    std::vector<int> indentStack = {0};
    bool atLineStart = true;

    void scanToken();
    char advance();
    char peek();
    char peekNext();
    bool match(char expected);
    bool isAtEnd();

    void addToken(TokenType type);
    void addToken(TokenType type, std::string literal);

    void number();
    void string();
    void identifier();
    void handleIndentation();
};