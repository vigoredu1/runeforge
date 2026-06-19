  cmake_minimum_required(VERSION 3.16)
  project(runeforge CXX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  include_directories(src)
  add_executable(runeforge
      src/main.cpp
      src/lexer.cpp
      src/parser.cpp
      src/interpreter.cpp
  )
