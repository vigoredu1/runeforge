#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

using namespace std;

int main(int argc, char* argv[]) {
      if (argc != 2) {
          std::cerr << "Usage: runeforge <file.rune>\n";
          return 1;
    }

  std::ifstream file(argv[1]);
  std::stringstream buf;
  buf << file.rdbuf();
  std::string source = buf.str();
try {
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.scanTokens();

  Parser parser(tokens);
  std::unique_ptr<Program> program = parser.parse();

  Interpreter interp;
  interp.interpret(*program);
} catch (std::runtime_error& e) {
      std::cerr << e.what() << "\n";
      return 1;
  }

}