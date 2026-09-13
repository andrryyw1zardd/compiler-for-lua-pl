#include <fstream>            // INCLUDED FOR std::fstream, readFile() 
#include <string>             // INCLUDED FOR std::pmr::string, std::getline()
#include <vector>             // INCLUDED FOR std::vector<Token>
#include "lexer/lexer.hpp"    // INCLUDED FOR class Lexer, tokenize() 
#include "parser/parser.hpp"  // INCLUDED FOR class Parser, parse_block()
#include <chrono>             // INCLUDED FOR high_resolution_clock::now() and duration_cast<microseconds>().count()
#include <iostream>           // INCLUDED FOR std::cerr
#include "sema/sema.hpp"

int main([[maybe_unused]]int argc, char** args) {
    std::string sourceCode;

    std::fstream readFile(args[1]);  
    std::pmr::string i;

    // reading and adding into sourceCode line by line (NOT char by char)
    while (std::getline(readFile, i)) { 
        sourceCode += i + "\n"; 
    }
    readFile.close(); 

    auto t0 = std::chrono::high_resolution_clock::now();
    Lexer lex{sourceCode};
    std::vector<Token> VectorOfTokens = lex.tokenize();

    auto t1 = std::chrono::high_resolution_clock::now();
    Parser parser{VectorOfTokens};  
    std::vector<Node*, ArenaAllocator<Node*>> ast = parser.parse_block();

    auto t2 = std::chrono::high_resolution_clock::now();
    DiagnosticEngine diags;
    SemanticAnalyzer sema{diags};
    for (const auto& node: ast) {
        node->accept(sema);
    }

    auto t3 = std::chrono::high_resolution_clock::now();

    auto lex_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    auto par_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    auto sem_us = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();

    if (parser.diag_count() == 0 && sema.diag_count() == 0) {
        std::cerr << "Lexer Speed: " << lex_us << " us\n";
        std::cerr << "Parser Speed: " << par_us << " us\n";
        std::cerr << "Sema Speed: " << sem_us << " us\n";
    }
    else { 
        parser.print_diags();
        sema.print_diags();
        return 1;
    }

    return 0;
}
