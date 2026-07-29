#include "parser/parser.hpp"
#include <string_view>
#include <format>
#include <algorithm>

Token Parser::peek() {
    if (index >= listOfTokens.size()) return Token{.type = Type::END_OF_FILE};
    return listOfTokens[index];
}

Token Parser::peekNext() {
    if (index + 1 >= listOfTokens.size()) return Token{.type = Type::END_OF_FILE};  
    return listOfTokens[index + 1];
}

Token Parser::advance() {
    if (index >= listOfTokens.size()) return Token{.type = Type::END_OF_FILE};  
    return listOfTokens[index++];
}

bool Parser::check(Type type) {
    if (peek().type == type) return true;
    return false;
}

bool Parser::checkNext(Type type) {
    if (peekNext().type == type) {
        return true;
    }
    return false;
}

void Parser::expect(Type type) {
    if (peek().type == type) {
        advance();
        return;
    }

    throwError(type);
}

void Parser::throwError(Type expected) {
    float positionX = peek().position.x;
    float positionY = peek().position.y;

    std::string expectedStr;
    std::string actualStr;

    std::string_view exp = TokenToStrArray[static_cast<size_t>(expected)];

    if (std::ranges::find(TokenToStrArray, exp) != TokenToStrArray.end()) expectedStr = exp;
    else throw std::runtime_error(
        std::format("internal compiler error: type not found in tokentostrmap in line {}, col {}",
                    positionY, positionX)
    );

    std::string_view act = TokenToStrArray[static_cast<size_t>(peek().type)];

    if (std::ranges::find(TokenToStrArray, act) != TokenToStrArray.end()) actualStr = act;
    else throw std::runtime_error(
        std::format("internal compiler error: type not found in tokentostrmap in line {}, col {}",
                    positionY, positionX)
    );

    throw std::runtime_error(
        std::format("Runtime Error at line {} col {}: expected {} but got {}",
                    positionY, positionX,
                    expectedStr, actualStr)
    );
}

bool Parser::endblock() {
    for (Type type : endblockArray) {
        if (check(type)) return true;
    }
    return false;
}

std::unique_ptr<Node> Parser::parse_ident() {
    std::vector<std::unique_ptr<Node>> left_side;

    std::unique_ptr<Node> left = parse_expr(0);
    left_side.push_back(std::move(left));

    while (check(Type::COMMA)) {
        if (check(Type::COMMA)) advance();

        left_side.push_back(parse_expr(0));
    }

    if (check(Type::EQUAL)) {
        advance();
        std::vector<std::unique_ptr<Node>> right_side;

        std::unique_ptr<Node> right = parse_expr(0);
        right_side.push_back(std::move(right));

        while (check(Type::COMMA)) {
            if (check(Type::COMMA)) advance();

            right_side.push_back(parse_expr(0));
        }

        return std::make_unique<MultipleVariableNode>(Type::EQUAL, std::move(left_side), std::move(right_side));
    }

    if (left_side.size() == 1 && 
       (left_side.front()->getName() == "FunctionCallNode" || 
        left_side.front()->getName() == "MethodCallNode"))
        return std::move(left_side.front());

    throwError(Type::IDENT);
    return nullptr;
}

std::unique_ptr<Node> Parser::parse_local() {
    advance();
    bool isConst = false;

    if (!check(Type::IDENT)) {
        if (check(Type::KW_FUNCTION)) {
            return parse_function(true);
        }
        if (check(Type::KW_CONST)) {
            isConst = true;
            advance();
        }
        else throwError(Type::KW_FUNCTION);
    }

    Token name = peek();
    std::unique_ptr<Node> expr = nullptr;

    std::vector<std::unique_ptr<Node>> left_side;
    std::vector<std::unique_ptr<Node>> right_side;

    if (checkNext(Type::COMMA)) {
        std::unique_ptr<Node> left = parse_expr(0);
        left_side.push_back(std::move(left));

        while (check(Type::COMMA)) {
            if (check(Type::COMMA)) advance();
            left_side.push_back(parse_expr(0));
        }

        if (check(Type::EQUAL)) {
            advance();

            std::unique_ptr<Node> right = parse_expr(0);
            right_side.push_back(std::move(right));

            if (check(Type::COMMA)) {
                while (check(Type::COMMA)) {
                    if (check(Type::COMMA)) advance();
                    right_side.push_back(parse_expr(0));
                }

                return std::make_unique<MultipleVariableNode>(
                    Type::EQUAL,
                    std::move(left_side), 
                    std::move(right_side)
                );
            }
            else {
                return std::make_unique<MultipleVariableNode>(
                    Type::EQUAL,
                    std::move(left_side),
                    std::move(right_side)
                );
            }
        }
        else {
            return std::make_unique<MultipleVariableNode>(
                Type::EQUAL,
                std::move(left_side),
                std::move(right_side)
            );
        }
    }
    else if (checkNext(Type::EQUAL)) {
        advance();
        advance();
        expr = parse_expr(0);

        return std::make_unique<DefineVariableNode>( std::move(name), std::move(expr) );
    }
    else if (checkNext(Type::LESS)) {
        advance();
        advance();
        Type type;

        if (check(Type::KW_CLOSE) || check(Type::KW_CONST)) {
            type = peek().type; 
            advance();
        }
        else throwError(Type::KW_CLOSE);

        expect(Type::GREATER);
        expect(Type::EQUAL);

        auto right = parse_expr(0);

        return std::make_unique<VarWithAttributeNode>(
            std::move(type),
            std::move(name),
            std::move(right)
        );
    }
    if (isConst && !checkNext(Type::EQUAL)) {
        throwError(Type::EQUAL);
    }
    else advance();

    return std::make_unique<DefineVariableNode>(
        std::move(name),
        std::move(expr)
    );
}

std::unique_ptr<Node> Parser::parse_do() {
    advance();

    auto body = parse_block();
    expect(Type::KW_END);

    return std::make_unique<DoNode>(std::move(body));
}

std::unique_ptr<Node> Parser::parse_while() {
    advance();

    std::unique_ptr<Node> condition = parse_expr(0); 

    if (!check(Type::KW_DO)) throwError(Type::KW_DO);
    advance();
    
    std::vector<std::unique_ptr<Node>> body = parse_block();
    expect(Type::KW_END);

    return std::make_unique<WhileNode>(
        std::move(condition),
        std::move(body) 
    );
}


std::unique_ptr<Node> Parser::parse_for() {
    advance();

    std::unique_ptr<Node> start;
    std::unique_ptr<Node> finish;
    std::unique_ptr<Node> step;

    if (!check(Type::IDENT)) { throwError(Type::IDENT); }
    Token var = peek();

    advance();

    if (check(Type::COMMA) || check(Type::KW_IN)) {
        std::vector<Token> keyArgs;
        keyArgs.push_back(var);

        while (check(Type::COMMA)) {
            advance();
            keyArgs.push_back(advance());
        }

        expect(Type::KW_IN);

        std::unique_ptr<Node> iter_fn = parse_ident();
        if (!(iter_fn->getName() == "FunctionCallNode")) throwError(Type::CALLEDFUNCTION);

        expect(Type::KW_DO);

        std::vector<std::unique_ptr<Node>> body = parse_block();
        expect(Type::KW_END);

        return std::make_unique<GenericForNode>(
            std::move(keyArgs), std::move(iter_fn), std::move(body)
        );
    }

    expect(Type::EQUAL);

    start = parse_expr(0);
    expect(Type::COMMA);

    finish = parse_expr(0);

    if (check(Type::COMMA)) {
        advance();
        step = parse_expr(0);
    }

    expect(Type::KW_DO);

    std::vector<std::unique_ptr<Node>> body = parse_block();
    expect(Type::KW_END);

    return std::make_unique<NumericForNode>(
        std::move(var), std::move(start), std::move(finish), std::move(step),
        std::move(body) 
    );
}

std::unique_ptr<Node> Parser::parse_if() {
    advance();

    std::unique_ptr<Node> condition = parse_expr(0); 

    if (check(Type::KW_THEN)) {
        advance();
    } else throwError(Type::KW_THEN);

    std::vector<std::unique_ptr<Node>> body = parse_block(); 

    std::vector<std::unique_ptr<Node>> elseifs;
    while (peek().type == Type::KW_ELSEIF) {
        elseifs.push_back(parse_elseif());
    }

    std::vector<std::unique_ptr<Node>> elseBody;
    while (peek().type == Type::KW_ELSE) {
        advance();
        elseBody = parse_block();
    }

    expect(Type::KW_END);

    return std::make_unique<IfNode>(
        std::move(condition),
        std::move(body), 
        std::move(elseifs),
        std::move(elseBody)
    );
}

std::unique_ptr<Node> Parser::parse_elseif() {
    advance();

    std::unique_ptr<Node> condition = parse_expr(0);

    expect(Type::KW_THEN);

    std::vector<std::unique_ptr<Node>> body = parse_block(); 

    return std::make_unique<ElseIfNode>(
        std::move(condition),
        std::move(body)
    );
}

std::unique_ptr<Node> Parser::parse_repeat() {
    advance();
    std::vector<std::unique_ptr<Node>> body = parse_block();

    expect(Type::KW_UNTIL);

    std::unique_ptr<Node> condition = parse_expr(0);

    return std::make_unique<RepeatUntilNode>(
        std::move(condition), 
        std::move(body)
    );
}

std::unique_ptr<Node> Parser::parse_function(bool isLocal) {
    advance();
    
    if (!check(Type::IDENT)) {
        throwError(Type::IDENT);
    }

    Token funcName = advance();

    std::vector<std::unique_ptr<Node>> Args;

    if (check(Type::L_PAREN)) {
        advance();
        std::unique_ptr<Node> args;

        while (!check(Type::R_PAREN)) {
            Args.push_back(parse_expr(0));

            if (check(Type::COMMA)) advance();
        }
        expect(Type::R_PAREN);
    }
    else {
        if (check(Type::DOT) || check(Type::COLON)) {
            advance();
            return parse_method(funcName, isLocal); 
        }
        else throwError(Type::COLON);
    } 
    
    std::vector<std::unique_ptr<Node>> body = parse_block(); 
    expect(Type::KW_END);

    return std::make_unique<FunctionNode>(
        std::move(funcName),
        isLocal,
        std::move(Args),
        std::move(body)
    );
}

std::unique_ptr<Node> Parser::parse_method(Token className, bool isLocal) {
    Token methodName = advance();
    std::vector<std::unique_ptr<Node>> args;

    if (check(Type::L_PAREN)) {
        advance();

        while (!check(Type::R_PAREN)) {
            args.push_back(parse_expr(0));

            if (!check(Type::COMMA) && !check(Type::R_PAREN)) { throwError(Type::R_PAREN); }
            if (check(Type::COMMA)) advance();
        }

        expect(Type::R_PAREN);
    }
    else throwError(Type::L_PAREN);
    
    std::vector<std::unique_ptr<Node>> body = parse_block(); 
    expect(Type::KW_END);

    return std::make_unique<MethodNode>(
        std::move(methodName),
        std::move(className),
        isLocal,
        std::move(args),
        std::move(body)
    );
}

std::unique_ptr<Node> Parser::parse_return() {
    advance();
    std::vector<std::unique_ptr<Node>> args;

    while (!endblock()) {
        if (check(Type::KW_FUNCTION)) {
            advance();
            expect(Type::L_PAREN);

            std::vector<std::unique_ptr<Node>> innerArgs;

            while (!check(Type::R_PAREN)) {
                innerArgs.push_back(parse_expr(0));

                if (!check(Type::COMMA) && !check(Type::R_PAREN)) { throwError(Type::R_PAREN); }
                if (check(Type::COMMA)) advance();
            }

            expect(Type::R_PAREN);

            std::vector<std::unique_ptr<Node>> body = parse_block();
            expect(Type::KW_END);

            return std::make_unique<AnonFunction>(
                std::move(innerArgs), std::move(body)
            );
        }

        args.push_back(parse_expr(0));

        if (check(Type::COMMA)) advance();
        else break;
    }

    return std::make_unique<ReturnNode>(std::move(args));
}

std::vector<std::unique_ptr<Node>> Parser::parse_block() {
    std::vector<std::unique_ptr<Node>> block;

    while (!endblock() && !check(Type::END_OF_FILE)) {
        std::unique_ptr<Node> statement = parse_stat(); 
        if (statement) {
            block.push_back(std::move(statement)); 
        }
    }

    return block;
}

std::unique_ptr<Node> Parser::parse_stat() {
    switch (peek().type) {
        case Type::KW_LOCAL:
            return parse_local();
            break;
        case Type::KW_IF:
            return parse_if();
            break;
        case Type::KW_WHILE:
            return parse_while();
            break;
        case Type::KW_FOR:
            return parse_for();
            break;
        case Type::KW_FUNCTION:
            return parse_function(false);
            break;
        case Type::IDENT:
            return parse_ident();
            break;
        case Type::KW_RETURN:
            return parse_return();
            break;
        case Type::KW_REPEAT:
            return parse_repeat();
            break;
        case Type::KW_DO:
            return parse_do();
            break;
        default:
            return parse_expr(0); 
            break;
    } 
}

int Parser::get_lbp() {
    switch (peek().type) {
        case Type::KW_OR:         return 10;
        case Type::KW_AND:        return 20;
        case Type::VERTICAL_BAR:  return 30;
        case Type::TILDE:         return 30;
        case Type::AMPERSAND:     return 30;
        case Type::L_SHIFT:       return 30;
        case Type::R_SHIFT:       return 30;
        case Type::EQUAL_EQUAL:   return 40;
        case Type::NOT_EQUAL:     return 40;
        case Type::GREATER:       return 50;
        case Type::GREATER_EQUAL: return 50;
        case Type::LESS:          return 50;
        case Type::LESS_EQUAL:    return 50;
        case Type::CONCAT:        return 60;
        case Type::PLUS:          return 70;
        case Type::MINUS:         return 70;
        case Type::PERCENT:       return 80;
        case Type::DOUBLE_SLASH:  return 80;
        case Type::SLASH:         return 80;
        case Type::CARET:         return 90;
        case Type::STAR:
            if (checkNext(Type::STAR)) return 90;
            return 80;
        case Type::NOT:           return 100;
        case Type::DOT:           return 110;
        case Type::L_PAREN:       return 110;
        case Type::L_BRACKET:     return 110;
        case Type::COLON:         return 110;
        default: return 0;
    }
}

std::unique_ptr<Node> Parser::nud() {
    if (check(Type::LIT_INT) || check(Type::LIT_FLOAT) || check(Type::LIT_STRING)
        || check(Type::KW_TRUE) || check(Type::KW_FALSE) || check(Type::LIT_LONG_STRING)
        || check(Type::LIT_HEX) || check(Type::LIT_CHAR) || check(Type::ELLIPSIS) 
        || check(Type::KW_NIL)
    ) {
        Token value = peek();
        advance();

        return std::make_unique<BasicDataNode>(std::move(value));
    }
    else if (std::ranges::find(UnaryOpSet, peek().type) != UnaryOpSet.end()) {
        Token op = advance();

        auto val = parse_expr(90);

        return std::make_unique<UnaryOpNode>(std::move(op), std::move(val));
    }
    else if (check(Type::L_PAREN)) {
        advance();

        auto inner_expr = parse_expr(0);
        expect(Type::R_PAREN);

        return inner_expr;
    }
    else if (check(Type::L_BRACE)) {
        advance();
        std::vector<std::unique_ptr<Node>> elements;

        while (!check(Type::R_BRACE)) {
            if (check(Type::L_BRACKET)) {
                advance();

                auto index_expr = parse_expr(0);
                auto index = std::make_unique<IndexNode>(std::move(index_expr));

                expect(Type::R_BRACKET);

                if (check(Type::EQUAL)) {
                    advance();

                    auto value = parse_expr(0);

                    elements.push_back(std::make_unique<TableFieldNode>(
                        std::move(index), std::move(value)));
                }
                else elements.push_back(std::move(index));
            }
            else { // for any other case
                auto key = parse_expr(0);

                if (check(Type::EQUAL)) {
                    advance();
                    auto value = parse_expr(0);

                    elements.push_back(std::make_unique<TableFieldNode>(
                        std::move(key), std::move(value)));
                }
                else elements.push_back(std::move(key));
            }

            if (check(Type::COMMA)) advance();
        } 
        expect(Type::R_BRACE);

        return std::make_unique<ArrayNode>(std::move(elements));
    }
    else if (check(Type::IDENT)) {
        Token value = peek();
        advance();

        return std::make_unique<VariableNode>(std::move(value));
    }
    else if (check(Type::KW_FUNCTION)) {
        advance();
        expect(Type::L_PAREN);

        std::vector<std::unique_ptr<Node>> innerArgs;

        while (!check(Type::R_PAREN)) {
            innerArgs.push_back(parse_expr(0));

            if (!check(Type::COMMA) && !check(Type::R_PAREN)) {
                throwError(Type::R_PAREN); 
            }

            if (check(Type::COMMA)) advance();
        }

        expect(Type::R_PAREN);

        std::vector<std::unique_ptr<Node>> body = parse_block();
        expect(Type::KW_END);

        return std::make_unique<AnonFunction>(
            std::move(innerArgs), std::move(body)
        );
    }

    throwError(Type::IDENT);
}

std::unique_ptr<Node> Parser::parse_expr(int min_lbp) {
    std::unique_ptr<Node> left = nud(); 
    
    while (true) {
        int lbp = get_lbp();
        if (lbp <= min_lbp) break;

        Type op = peek().type;

        if (op == Type::STAR && peekNext().type == Type::STAR) {
            advance(); advance();

            std::unique_ptr<Node> right = parse_expr(lbp - 1); 
            left = std::make_unique<BinaryOpNode>(
                op,
                std::move(left),
                std::move(right)
            );

            continue;
        }

        if (op == Type::CONCAT) {
            advance();

            std::unique_ptr<Node> right = parse_expr(lbp - 1); 

            left = std::make_unique<BinaryOpNode>(
                op,
                std::move(left),
                std::move(right)
            );
            continue;
        }

        if (op == Type::DOT) {
            advance();

            if (!check(Type::IDENT)) throwError(Type::IDENT);
            Token q = advance();

            left = std::make_unique<MemberAccessNode>(std::move(left), std::move(q));
            continue;
        }

        if (op == Type::L_PAREN) {
            advance(); 

            std::vector<std::unique_ptr<Node>> args;

            while (!check(Type::R_PAREN)) {
                args.push_back(parse_expr(0));

                if (check(Type::COMMA)) advance();
            }

            expect(Type::R_PAREN); 

            left = std::make_unique<FunctionCallNode>(
                std::move(left),
                std::move(args)
            );
            continue;
        }

        if (op == Type::COLON) {
            advance();
            Token method_name = advance();

            std::vector<std::unique_ptr<Node>> args;
            expect(Type::L_PAREN);

            while (!check(Type::R_PAREN)) {
                args.push_back(parse_expr(0));
                if (check(Type::COMMA)) advance();
            }

            expect(Type::R_PAREN); 

            left = std::make_unique<MethodCallNode>(
                std::move(method_name), 
                std::move(left),
                std::move(args)
            );
            continue;
        }

        // can parse only a[i] but not a[i][j] and so on
        if (op == Type::L_BRACKET) {
            advance();

            auto index_expr = parse_expr(lbp);
            expect(Type::R_BRACKET);

            left = std::make_unique<ExprWithIndexNode>(
                std::move(left),
                std::move(index_expr)
            );
            continue;
        }

        if (op == Type::KW_AND) {
            advance();
            auto right = parse_expr(lbp);

            left = std::make_unique<AndTernaryNode>(
                std::move(left),
                std::move(right)
            );
            continue;
        }

        if (op == Type::KW_OR) {
            advance();
            auto right = parse_expr(lbp);

            left = std::make_unique<OrTernaryNode>(
                std::move(left),
                std::move(right)
            );
            continue;
        }

        if (std::ranges::find(BitwiseOpSet, op) != BitwiseOpSet.end()) {
            advance();
            auto right = parse_expr(lbp);

            left = std::make_unique<BitwiseNode>(
                std::move(op),
                std::move(left),
                std::move(right)
            );
            continue;
        }

        if (lbp > 0) {
            advance();

            std::unique_ptr<Node> right = parse_expr(lbp);

            left = std::make_unique<BinaryOpNode>(
                op,
                std::move(left),
                std::move(right)
            );
        }
        else break;
    }

    return left;
}
