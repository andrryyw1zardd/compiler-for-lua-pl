cmake_minimum_required(VERSION 3.20)
project(lua_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD            23)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
  lua++
  main.cpp
  src/lexer.cpp
  src/parser.cpp
)

target_include_directories(lua++ PRIVATE include)
target_compile_options(lua++ PRIVATE -Wall -Wextra)
