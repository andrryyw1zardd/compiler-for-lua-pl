/* -------------------------------------------------------------------------------------------------------------
// DONE
Lexer (Scanner / Tokenizer) 
{
    This is the first component that reads the raw source code and breaks it down into small,
    meaningful chunks called TOKENS.
    For example, it identifies KEYWORDS (like int), IDENTIFIERS (variable names), and SYMBOLS (like + or {).
}

// DONE 
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
#include <fstream>         // INCLUDED FOR std::fstream, readFile() 
#include <string>          // INCLUDED FOR std::pmr::string, std::getline()
#include <vector>          // INCLUDED FOR std::vector<Token>
#include "lexer.hpp"       // INCLUDED FOR class Lexer, tokenize() 
#include "parser.hpp"      // INCLUDED FOR class Parser, parse_block()

int main([[maybe_unused]]int argc, char** args) {
  std::string sourceCode;

  std::fstream readFile(args[1]);  
  std::pmr::string i;

  // reading and adding into sourceCode line by line (NOT char by char)
  while (std::getline(readFile, i)) { 
    sourceCode += i + "\n"; 
  }

  Lexer lex(sourceCode);
  std::vector<Token> VectorOfTokens = lex.tokenize();

  Parser parser(VectorOfTokens);  
  auto ast = parser.parse_block();

  readFile.close(); 
  return 0;
}
