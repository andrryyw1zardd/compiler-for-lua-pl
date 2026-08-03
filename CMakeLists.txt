cmake_minimum_required(VERSION 3.20)
project(lua_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD            23)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(
    lua++
    main.cpp
    src/sema/sema.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/allocator/alloc.cpp
)

target_include_directories(lua++ PRIVATE include)
target_compile_options(lua++ PRIVATE -Wall -Wextra)
