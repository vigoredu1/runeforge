#pragma once
#include <string>

enum class TokenType {
    RUNE, SPELL, CHANNEL, WHEN, ORWHEN, OTHERWISE,
    WHILE, SUMMON, FROM, BLESSED, CURSED,
    AND, OR, NOT, RITUAL, UNLEASH,
    IDENTIFIER, NUMBER, STRING,
    PLUS, MINUS, STAR, SLASH,
    EQUAL, EQUAL_EQUAL, GREATER, LESS,
    LPAREN, RPAREN, COLON, COMMA,
    INDENT, DEDENT, NEWLINE, END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};