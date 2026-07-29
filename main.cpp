/* -------------------------------------------------------------------------------------------------------------
// DONE
Lexer (Scanner / Tokenizer) 
{
    This is the first component that reads the raw source code and breaks it down into small,
    meaningful chunks called TOKENS.
    For example, it identifies KEYWORDS (like int), IDENTIFIERS (variable names), and SYMBOLS (like + or {).
}

Parser 
{
    The parser takes the stream of tokens and organizes them into a tree structure called an
    ABSTRACT SYNTAX TREE (AST).
    This tree represents the logical structure of the code, 
    ensuring it follows the rules of the LANGUAGE'S GRAMMAR.
    RECURSIVE DESCENT and PRATT parsing methods
}

Symbol Table 
{
    A data structure (often a hash map) used throughout the compilation process
    to store information about variables, functions, and classes, such as their types and scopes.
}

Semantic Analyzer (Type Checker) 
{
    This component checks the AST against the symbol table to catch logical errors
    that aren't syntax-related, such as adding a string to an integer or using an undeclared variable.
}

Intermediate Representation (IR) Generator 
{
    Instead of translating directly to machine code,
    many compilers first convert the AST into a platform-independent Intermediate Representation (IR).
    This makes it easier to optimize the code before the final translation.
}

Code Generator 
{
    The final component translates the IR (or AST) into the target language
    typically assembly code or machine code—for a specific processor.
}
------------------------------------------------------------------------------------------------------------- */
#include <fstream>            // INCLUDED FOR std::fstream, readFile() 
#include <string>             // INCLUDED FOR std::pmr::string, std::getline()
#include <vector>             // INCLUDED FOR std::vector<Token>
#include "lexer/lexer.hpp"    // INCLUDED FOR class Lexer, tokenize() 
#include "parser/parser.hpp"  // INCLUDED FOR class Parser, parse_block()
#include <chrono>
#include <iostream>

int main([[maybe_unused]]int argc, char** args) {
    std::string sourceCode;

    auto t0 = std::chrono::high_resolution_clock::now();

    std::fstream readFile(args[1]);  
    std::pmr::string i;

    // reading and adding into sourceCode line by line (NOT char by char)
    while (std::getline(readFile, i)) { 
        sourceCode += i + "\n"; 
    }

    Lexer lex{sourceCode};
    std::vector<Token> VectorOfTokens = lex.tokenize();

    auto t1 = std::chrono::high_resolution_clock::now();

    Parser parser{VectorOfTokens};  
    auto ast = parser.parse_block();

    auto t2 = std::chrono::high_resolution_clock::now();

    auto lex_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    auto par_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    std::cerr << "Lexer Speed: " << lex_us << " us\n";
    std::cerr << "Parser Speed: " << par_us << " us\n";

    readFile.close(); 
    return 0;
}

// Ok so making ast with bunch of smart pointers wasn't a good idea
// But now I know about Arena Allocation and I want to make my own Allocator
// So I can use it to make my compiler faster and to save memory 
// Heres what we got now --- Lexer Speed: 1220 us ; Parser Speed: 3476 us
// 0.001 + 0.003 = 0.004 seconds, which is extremely slow
