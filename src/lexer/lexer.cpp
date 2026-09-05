#include "lexer/lexer.hpp"
#include <cctype>
#include <format>
#include <algorithm>

char Lexer::peek() {
    if (index >= sourceCode.size()) return '\0';
    return sourceCode[index];
}

char Lexer::peekPast() {
    if (index >= sourceCode.size()) return '\0';
    return sourceCode[index-1];
}

char Lexer::peekNext() {
    if (index+1 >= sourceCode.size()) return '\0';
    return sourceCode[index+1];
}

char Lexer::peekNextNext() {
    if (index+1 >= sourceCode.size()) return '\0';
    return sourceCode[index+2];
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

bool Lexer::isComment() {
    if (peek() == '-' && peekNext() == '-') {
        return true;
    }

    return false;
}

void Lexer::skipWhiteSpace() {
    while (index < sourceCode.size()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            advance();
        else break;
    }
}

void Lexer::throwError(std::string reason) {
    throw std::runtime_error(
        std::format("Runtime Error at line {} col {}\n Reason: {}", 
                    y, x, reason)
    );
}

Token Lexer::nextToken() {
    skipWhiteSpace();

    if (isComment()) {
        advance(); advance();

        // multiple line comment
        if (peek() == '[') {
            int level = 0;
            int savedX = x;
            int savedY = y;
            int savedIndex = index;

            advance();

            while (peek() == '=') {
                level++;
                advance();
            }

            if (peek() == '[') {
                advance();

                while (true) {
                    if (peek() == '\0')
                        throwError("Didnt close the comment");

                    if (peek() == ']') {
                        advance();
                        int innerLevel = 0;

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
            } else { 
                x = savedX; 
                y = savedY;
                index = savedIndex;
            }
        }

        // single line comment
        while (peek() != '\n' && peek() != '\0') advance();
        return nextToken();
    }

    // just eating it all until operator 
    std::string value;

   // this statment is just for ident to not begin from number (123var)
   if (std::isalpha(peek()) || peek() == '_') { 

        size_t start = index;
        while (std::isalnum(peek()) || peek() == '_') { advance(); }
        value = sourceCode.substr(start, index - start);
    }

    if (value != "") {
        auto t = keywordMap.find(value);
        if (t != keywordMap.end()) { // this is keyword
            Type type = t->second;

            return Token{.type = type, .value = value, .position = {x, y}};
        } 
        // this is just a identificator
        else return Token{.type = Type::IDENT, .value = value, .position = {x, y}};
    }

    { // string check 
        if (peek() == '"') {
            advance();

            while (peek() != '"' && peek() != '\0') {
                value += advance();
            }
            if (peek() == '\0') throwError("Unterminated string");
            advance();
        }
        
        if (value != "") {
            return Token{.type = Type::LIT_STRING, .value = value, .position = {x, y}};
        }
    }

    { // single quoted string check
        if (peek() == '\'') {
            advance();

            while (peek() != '\'' && peek() != '\0') {
                value += advance();
            }
            if (peek() == '\0') throwError("Unterminated char literal");
            advance();
        }

        if (value != "") {
            return Token{.type = Type::LIT_CHAR, .value = value, .position = {x, y}};
        }
    }

    { // long string check
        if (peek() == '[') {
            int level = 0;
            int savedX = x;
            int savedY = y;
            int savedIndex = index;

            advance(); 

            while (peek() == '=') {
                level++;
                advance();
            }

            if (peek() == '[') {
                advance();

                while (true) {
                    if (peek() == '\0')
                        throwError("Didn't close the long string quotes aka ]]"); 

                    if (peek() == ']') {
                        int innerLevel = 0;
                        advance(); 

                        while (peek() == '=') {
                            innerLevel++;
                            advance();
                        }

                        if (peek() == ']' && level == innerLevel) {
                            advance();
                            return Token{.type = Type::LIT_LONG_STRING, .value = value, .position = {x, y}};
                        }

                        value += ']';
                        value += std::string(innerLevel, '=');
                        continue;
                    }
                    value += advance();
                }
            } else {
                if (level != 0) throwError("wrong use of long string quotes");
                x = savedX;
                y = savedY;
                index = savedIndex;
            }
        }
    }

    // checking for hex literals
    if (peek() == '0' && (peekNext() == 'x' || peekNext() == 'b' || peekNext() == 'o')) {
        advance();
        std::string value;
        int detect;

        if (peek() == 'b') {
            advance();
            detect = 2;

            if (peek() != '0' && peek() != '1') {
                return Token{.type = Type::ERROR, .value = value, .position = {x, y}};
            }

            while (peek() == '0' || peek() == '1') {
                value += advance();
            }
        }

        else if (peek() == 'x') {
            advance();
            detect = 16;

            std::vector<char> int_digits = {'0','1','2','3','4','5','6','7', '8', '9'};
            std::vector<char> lower_str_digits = {'a','b','c','d','e','f'};
            std::vector<char> upper_str_digits = {'A','B','C','D','E','F'};

            while (std::ranges::find(int_digits, peek()) != int_digits.end()
                || std::ranges::find(lower_str_digits, peek()) != lower_str_digits.end()
                || std::ranges::find(upper_str_digits, peek()) != upper_str_digits.end()
            ) {
                value += advance();
            }
        }
        else {
            advance();
            detect = 8;

            std::vector<char> digits = {'0','1','2','3','4','5','6','7'};

            while (std::ranges::find(digits, peek()) != digits.end()) {
                value += advance();
            }

        }

        return Token{.type = Type::LIT_HEX, .value = std::stoi(value, nullptr, detect), .position = {x, y}};
    }

   // checking for numeric literals (both int and float), but also numbers with exponent
    if (std::isdigit(peek()) || (peek() == '.' && std::isdigit(peekNext()))) {
        std::string value;
        bool isFloat = false;
        bool seenDot = false;
        bool seenExp = false;

        while (std::isdigit(peek()) || (peek() == 'e' || peek() == 'E') || peek() == '.' || (peek() == '+' || peek() == '-')) {
            if (std::isdigit(peek())) {
                value += advance();
                continue;
            }
            else if (peek() == '.') {
                if (!seenDot && !seenExp) {
                    seenDot = true;

                    value += advance();
                    continue;
                }

                return Token{.type = Type::ERROR, .position = {x, y}};
            }
            else if ((peek() == 'e' || peek() == 'E')) {
                if (std::isdigit(peekNext()) || ((peekNext() == '+' || peekNext() == '-') && std::isdigit(peekNextNext()))) {
                    seenExp = true;

                    value += advance();
                    continue;
                }

                return Token{.type = Type::ERROR, .position = {x, y}};
            }

            else if (peek() == '-' || peek() == '+') {
                if (peekPast() == 'e' || peekPast() == 'E') {
                    value += advance();
                    continue;
                }

                isFloat = seenDot || seenExp;
                if (isFloat) { 
                    return Token{.type = Type::LIT_FLOAT, .value = std::stof(value), .position = {x, y}};
                } else { 
                    return Token{.type = Type::LIT_INT, .value = std::stoi(value), .position = {x, y}};
                }
            }
        }

        // converting and returning value
        isFloat = seenDot || seenExp;

        if (isFloat) { 
            return Token{.type = Type::LIT_FLOAT, .value = std::stof(value), .position = {x, y}};
        } else { 
            return Token{.type = Type::LIT_INT, .value = std::stoi(value), .position = {x, y}};
        }
    }

    // checking if its a symbol   
    Type type = operationTable[static_cast<unsigned char>(peek())];

    if (type != Type::ERROR) { // if exists in array 
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
                else if (match('<')) type = Type::L_SHIFT;
                else type = Type::LESS;
                return Token{.type = type, .position = {x, y}};

            case '>':
                advance();
                if (match('=')) type = Type::GREATER_EQUAL;
                else if (match('>')) type = Type::R_SHIFT;
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
                type = Type::DOT;
                if (peek() == '.' && peekNext() == '.') {
                    advance(); advance();
                    type = Type::ELLIPSIS;
                }
                if (peek() == '.') {
                    advance();
                    type = Type::CONCAT;
                }
                return Token{.type = type, .position = {x, y}};

            default:
                advance();
                return Token{.type = Type::ERROR, .position = {x, y}};
        }
    }
}

std::vector<Token> Lexer::tokenize() { 
    std::vector<Token> VectorOfTokens;
    VectorOfTokens.reserve(sourceCode.size() / 4);

    while (peek() != '\0') {
        Token t = nextToken();
        VectorOfTokens.push_back(t);
    } 

    return VectorOfTokens;
}
