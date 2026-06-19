# RuneForge

A fantasy-themed programming language implemented in C++. Indentation-sensitive syntax with keywords drawn from arcane / spellcasting lore.

> **Status:** Lexer complete. Parser and interpreter in progress.

## Language Syntax (Preview)

```runeforge
rune x = 5

spell greet(name):
    summon from io channel print
    print("Hello, " + name)

ritual main():
    when x > 3:
        greet("world")
    orwhen x == 0:
        greet("void")
    otherwise:
        greet("unknown")
```

### Keywords

| Keyword     | Role                        |
|-------------|-----------------------------|
| `rune`      | Variable declaration        |
| `spell`     | Function definition         |
| `ritual`    | Entry-point / special function |
| `channel`   | Module / import namespace   |
| `summon`    | Import statement            |
| `from`      | Used with `summon`          |
| `when`      | `if`                        |
| `orwhen`    | `elif`                      |
| `otherwise` | `else`                      |
| `while`     | Loop                        |
| `blessed`   | `true`                      |
| `cursed`    | `false`                     |
| `and`       | Logical AND                 |
| `or`        | Logical OR                  |
| `not`       | Logical NOT                 |

## Building

**Requirements:** CMake ≥ 3.28, GCC 13+ (or equivalent C++17 compiler), Ninja.

```bash
cmake -B build -G Ninja
cmake --build build
```

Binary lands at `build/runeforge`.

## Running

```bash
echo "rune x = 5" | ./build/runeforge
```

Currently prints raw token types and lexemes to stdout.

## Project Structure

```
src/
  token.h       — TokenType enum + Token struct
  lexer.h/.cpp  — Scanner: source → token stream
  main.cpp      — Driver (reads source, prints tokens)
CMakeLists.txt
```
