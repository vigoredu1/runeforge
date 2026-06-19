#include <string>
#include <lexer.h>
#include <iostream>
int main() {
    std::string src = "rune x = 5";
    Lexer lexer(src);
    auto tokens = lexer.scanTokens();
    for (auto& t : tokens) {
        std::cout << (int)t.type << " " << t.lexeme << "\n";
    }
}