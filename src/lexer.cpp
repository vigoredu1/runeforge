#include "lexer.h"
#include <unordered_map>
#include <cctype>

static std::unordered_map<std::string, TokenType> keywords = {
    {"rune", TokenType::RUNE},
    {"spell", TokenType::SPELL},
    {"ritual", TokenType::RITUAL},
    {"channel", TokenType::CHANNEL},
    {"when", TokenType::WHEN},
    {"orwhen", TokenType::ORWHEN},
    {"otherwise", TokenType::OTHERWISE},
    {"while", TokenType::WHILE},
    {"summon", TokenType::SUMMON},
    {"from", TokenType::FROM},
    {"blessed", TokenType::BLESSED},
    {"cursed", TokenType::CURSED},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
    {"unleash", TokenType::UNLEASH},
};

Lexer::Lexer(std::string source) : source(source) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    // close all open indent levels before EOF
    while (indentStack.size() > 1) {
        indentStack.pop_back();
        tokens.push_back({TokenType::DEDENT, "", line});
    }
    tokens.push_back({TokenType::END_OF_FILE, "", line});
    return tokens;
}

bool Lexer::isAtEnd() {
    return current >= source.size();
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() {
    return isAtEnd() ? '\0' : source[current];
}

char Lexer::peekNext() {
    return (current + 1 >= source.size()) ? '\0' : source[current + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source[current] != expected) return false;
    current++;
    return true;
}

void Lexer::addToken(TokenType type) {
    addToken(type, source.substr(start, current - start));
}

void Lexer::addToken(TokenType type, std::string literal) {
    tokens.push_back({type, literal, line});
}

void Lexer::scanToken() {
    if (atLineStart) {
        handleIndentation();
        atLineStart = false;
    }

    char c = advance();
    switch (c) {
        case '+': addToken(TokenType::PLUS); break;
        case '-': addToken(TokenType::MINUS); break;
        case '*': addToken(TokenType::STAR); break;
        case '/': addToken(TokenType::SLASH); break;
        case '(': addToken(TokenType::LPAREN); break;
        case ')': addToken(TokenType::RPAREN); break;
        case ':': addToken(TokenType::COLON); break;
        case ',': addToken(TokenType::COMMA); break;
        case '=':
            addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
            break;
        case '>': addToken(TokenType::GREATER); break;
        case '<': addToken(TokenType::LESS); break;
        case '"': string(); break;
        case '\n':
            addToken(TokenType::NEWLINE);
            line++;
            atLineStart = true;
            break;
        case ' ': case '\r': case '\t': break;
        default:
            if (isdigit(c)) number();
            else if (isalpha(c) || c == '_') identifier();
    }
}

void Lexer::number() {
    while (isdigit(peek())) advance();
    if (peek() == '.' && isdigit(peekNext())) {
        advance();
        while (isdigit(peek())) advance();
    }
    addToken(TokenType::NUMBER);
}

void Lexer::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }
    advance();
    addToken(TokenType::STRING, source.substr(start + 1, current - start - 2));
}

void Lexer::identifier() {
    while (isalnum(peek()) || peek() == '_') advance();
    std::string text = source.substr(start, current - start);
    auto it = keywords.find(text);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    addToken(type);
}

void Lexer::handleIndentation() {
    int indent = 0;
    while (peek() == ' ') {
        advance();
        indent++;
    }
    if (peek() == '\n' || isAtEnd()) return;

    int prev = indentStack.back();
    if (indent > prev) {
        indentStack.push_back(indent);
        addToken(TokenType::INDENT);
    } else {
        while (indent < indentStack.back()) {
            indentStack.pop_back();
            addToken(TokenType::DEDENT);
        }
    }
    start = current;
}
