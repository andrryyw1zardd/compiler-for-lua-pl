#include "lexer.hpp"
#include <cctype>

char Lexer::peek() {
  if (index >= sourceCode.size()) return '\0';
  return sourceCode[index];
}

char Lexer::peekNext() {
  if (index+1 >= sourceCode.size()) return '\0';
  return sourceCode[index+1];
}

char Lexer::advance() {
  if (index >= sourceCode.size()) return '\0';
  char c = sourceCode[index];

  if (c == '\n') {
    x = 1;
    y++;
  } 
  else x++;
  index++;
  return c;
}

bool Lexer::match(char expected) {
  if (peek() != expected) return false;

  advance();
  return true;
}

void Lexer::skipWhiteSpace() {
  while (index < sourceCode.size()) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
      advance();
    else break;
  }
}

Token Lexer::nextToken() {
  skipWhiteSpace();

  if (peek() == '-' && peekNext() == '-') {
    advance(); advance();

    // multiple line comment
    if (peek() == '[') {
      int level = 0;
      int savedX = x;

      advance();

      while (peek() == '=') {
        level++;
        advance();
      }

      if (peek() == '[') {
        advance();

        while (true) {
          if (peek() == '\0') return Token{.type = Type::ERROR, .position = {x, y}};

          if (peek() == ']') {
            int innerLevel = 0;
            int innerX = x;

            advance();

            while (peek() == '=') {
              innerLevel++;
              advance();
            }

            if (peek() == ']' && level == innerLevel) {
              advance();
              return nextToken();
            }

            continue;
          }

          advance();
        }
      } else { x = savedX; }
    }

    // single line comment
    while (peek() != '\n' && peek() != '\0') advance();
    return nextToken();
  }

  // just eating it all until operator 
  std::string value;
  if (std::isalpha(peek()) || peek() == '_') { // this if statment is just for ident to not start from number (123var)
    while (std::isalnum(peek()) || peek() == '_') {
      value += peek();
      advance();
    }
  }

  if (value != "") {
    if (keywordMap.count(value)) { // this is keyword
      Type type;   
      type = keywordMap[value];

      return Token{.type = type, .value = value, .position = {x, y}};
    } 
    // this is just a identificator
    else return Token{.type = Type::IDENT, .value = value, .position = {x, y}};
  }

  // string check 
  if (peek() == '"') {
    advance();

    while (peek() != '"' && peek() != '\0') {
      value += peek();
      advance();
    }
    advance();
  }

  if (value != "") {
      return Token{.type = Type::LIT_STRING, .value = value, .position = {x, y}};
  }

  // checking if its a number (int/float)
  if (std::isdigit(peek())) {
    std::string value;
    bool isFloat = false;

    while (std::isdigit((unsigned char)peek())) {
      value += peek();
      advance();
    }

    if (peek() == '.') {
      isFloat = true;
      value += peek();
      advance();

      while (std::isdigit((unsigned char)peek())) {
        value += peek();
        advance();
      }
    }

    // converting and returning value
    if (isFloat) { 
      return Token{.type = Type::LIT_FLOAT, .value = std::stof(value), .position = {x, y}};
    } else { 
      return Token{.type = Type::LIT_INT, .value = std::stoi(value), .position = {x, y}};
    }
  }

  // checking if its a symbol  
  Type type;
  if (operationMap.count(peek())) { // if exists in map
    type = operationMap[peek()]; 
    advance();

    return Token{.type = type, .position = {x, y}};

  } else {
    switch (peek()) {
      case '=':
        advance();
        if (match('=')) type = Type::EQUAL_EQUAL;
        else type = Type::EQUAL;
        return Token{.type = type, .position = {x, y}};

      case '/':
        advance();
        if (match('/')) type = Type::DOUBLE_SLASH;
        else type = Type::SLASH;
        return Token{.type = type, .position = {x, y}};

      case '<':
        advance();
        if (match('=')) type = Type::LESS_EQUAL;
        else type = Type::LESS;
        return Token{.type = type, .position = {x, y}};

      case '>':
        advance();
        if (match('=')) type = Type::GREATER_EQUAL;
        else type = Type::GREATER;
        return Token{.type = type, .position = {x, y}};

      case ':':
        advance();
        if (match(':')) type = Type::COLON_COLON;
        else type = Type::COLON;
        return Token{.type = type, .position = {x, y}};

       case '~':
        advance();
        if (match('=')) type = Type::NOT_EQUAL;
        else type = Type::ERROR; 
        return Token{.type = type, .position = {x, y}};

      case '.':
        advance();
        if (match('.')) type = Type::CONCAT;
        else type = Type::DOT;
        return Token{.type = type, .position = {x, y}};

      case '|':
        advance();
        if (match('|')) type = Type::VERTICAL_BAR;
        else type = Type::ERROR;
        return Token{.type = type, .position = {x, y}};
      case '&':
        advance();
        if (match('&')) type = Type::AMPERSAND;
        else type = Type::ERROR;
        return Token{.type = type, .position = {x, y}};

      default:
        advance();
        return Token{.type = Type::ERROR, .position = {x, y}};
    }
  }
}

std::vector<Token> Lexer::tokenize() { 
  std::vector<Token> VectorOfTokens;

  while (peek() != '\0') {
    Token t = nextToken();
    VectorOfTokens.push_back(t);
  } 

  return VectorOfTokens;
}
