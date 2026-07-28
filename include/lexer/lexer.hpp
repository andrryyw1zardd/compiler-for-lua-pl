#include <any>            // included for std::any in struct Token
#include <string>         // included for std::string in class Lexer 
#include <vector>         // included for std::vector<Token> tokenize() in class Lexer 
#include <unordered_map>  // included for std::unordered_map<char, Type> operationMap in class Lexer 
#include <stdbool.h>      // included for bool match(char expected) in class Lexer 

#ifndef LEXER_HPP
#define LEXER_HPP

enum class Type {
  KW_LOCAL, KW_IF, KW_THEN, KW_ELSE, KW_ELSEIF, KW_END,
  KW_FUNCTION, KW_RETURN, KW_WHILE, KW_CONST, KW_CLOSE,
  KW_FOR, KW_DO, KW_REPEAT, KW_UNTIL, KW_NIL,
  KW_TRUE, KW_FALSE, KW_AND, KW_OR, KW_NOT, KW_IN,

  LIT_INT, // Literals
  LIT_FLOAT, LIT_STRING,
  LIT_HEX, LIT_CHAR, LIT_LONG_STRING,

  IDENT, CALLEDFUNCTION, // Identificator
  
  PLUS, // Symbols 
  MINUS, STAR, VERTICAL_BAR, SLASH, DOUBLE_SLASH, PERCENT,
  CARET, DOT, CONCAT, HASH, EQUAL, EQUAL_EQUAL,
  NOT_EQUAL, LESS, GREATER, LESS_EQUAL,
  GREATER_EQUAL, L_PAREN, R_PAREN, L_BRACE,
  R_BRACE, L_BRACKET, R_BRACKET, COMMA,
  COLON, COLON_COLON, SEMICOLON, AMPERSAND, 
  NOT, ELLIPSIS, TILDE, L_SHIFT, R_SHIFT,

  END_OF_FILE, // Specefic
  ERROR
};

struct Vect2 { int x; int y; };

struct Token { 
  Type type; 
  std::any value;
  Vect2 position;
};

class Lexer {
private:
  std::string sourceCode;
  unsigned long long int index;
  int x, y;

  static inline std::unordered_map<char, Type> operationMap = {
    {'+', Type::PLUS},
    {'-', Type::MINUS},
    {'*', Type::STAR},
    {'%', Type::PERCENT},
    {'^', Type::CARET},
    {'#', Type::HASH},
    {'(', Type::L_PAREN},
    {')', Type::R_PAREN},
    {'{', Type::L_BRACE},
    {'}', Type::R_BRACE},
    {'[', Type::L_BRACKET},
    {']', Type::R_BRACKET},
    {',', Type::COMMA},
    {';', Type::SEMICOLON},
    {'!', Type::NOT},
    {'&', Type::AMPERSAND},
    {'~', Type::TILDE},
    {'|', Type::VERTICAL_BAR},
    {'\0', Type::END_OF_FILE},
  };
  
  static inline std::unordered_map<std::string, Type> keywordMap = {
    {"local",    Type::KW_LOCAL},
    {"const",    Type::KW_CONST},
    {"close",    Type::KW_CLOSE},
    {"if",       Type::KW_IF},
    {"then",     Type::KW_THEN},
    {"else",     Type::KW_ELSE},
    {"elseif",   Type::KW_ELSEIF},
    {"end",      Type::KW_END},
    {"function", Type::KW_FUNCTION},
    {"return",   Type::KW_RETURN},
    {"while",    Type::KW_WHILE},
    {"for",      Type::KW_FOR},
    {"do",       Type::KW_DO},
    {"repeat",   Type::KW_REPEAT},
    {"until",    Type::KW_UNTIL},
    {"nil",      Type::KW_NIL},
    {"true",     Type::KW_TRUE},
    {"false",    Type::KW_FALSE},
    {"and",      Type::KW_AND},
    {"or",       Type::KW_OR},
    {"not",      Type::KW_NOT},
    {"in",       Type::KW_IN},
  };

  static constexpr char numbersArray[10] = {'0', '1', '2', '3', '4',
                          '5', '6', '7', '8', '9'};

  char peek(); 
  char peekPast();
  char peekNext(); 
  char peekNextNext(); 
  char advance(); 
  bool match(char expected); 
  bool isComment(); 
  void skipWhiteSpace(); 
  void throwError(std::string reason);

public:
  Lexer(std::string source) : sourceCode(source), index(0), x(1), y(1) {}
  Token nextToken(); 
  std::vector<Token> tokenize(); 
};

#endif
