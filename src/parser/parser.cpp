#include "parser/parser.hpp"
#include <string_view>
#include <format>
#include <algorithm>
#include <iostream>

Arena arena{1024 * 1024};
ArenaAllocator<Node*> alloc(arena);
ArenaAllocator<Type> alloc_for_type(arena);
ArenaAllocator<bool> alloc_for_bool(arena);

void Parser::free_arena() {
    arena.free();
}

const Token& Parser::peek() {
    static const Token eof {.type = Type::END_OF_FILE};
    if (index >= listOfTokens.size()) return eof;
    return listOfTokens[index];
}

const Token& Parser::peekNext() {
    static const Token eof {.type = Type::END_OF_FILE};
    if (index >= listOfTokens.size()) return eof;
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
    else parser_diag_vect.push_back(
        std::format(
            "internal compiler error: type not found in tokentostrmap in line {}, col {}",
            positionY, positionX)
    );

    std::string_view act = TokenToStrArray[static_cast<size_t>(peek().type)];

    if (std::ranges::find(TokenToStrArray, act) != TokenToStrArray.end()) actualStr = act;
    else parser_diag_vect.push_back( 
        std::format("internal compiler error: type not found in tokentostrmap in line {}, col {}",
                    positionY, positionX)
    );

    parser_diag_vect.push_back( 
        std::format("Runtime Error at line {} col {}: expected {} but got {}",
                    positionY, positionX,
                    expectedStr, actualStr)
    );
}

void Parser::print_diags() {
    // output of errors detected by the parser
    for (const auto& pdv: parser_diag_vect) {
        std::cout << pdv << std::endl;
    }
}

int Parser::diag_count() {
    return parser_diag_vect.size();
}

bool Parser::endblock() {
    for (Type type : endblockArray) {
        if (check(type)) return true;
    }
    return false;
}

Node* Parser::parse_ident() {
    std::vector<Node*, ArenaAllocator<Node*>> left_side(alloc);

    Node* left = parse_expr(0);
    left_side.push_back(left);

    while (check(Type::COMMA)) {
        if (check(Type::COMMA)) advance();

        left_side.push_back(parse_expr(0));
    }

    if (check(Type::EQUAL)) {
        advance();
        std::vector<Node*, ArenaAllocator<Node*>> right_side(alloc);

        Node* right = parse_expr(0);
        right_side.push_back(right);

        while (check(Type::COMMA)) {
            if (check(Type::COMMA)) advance();

            right_side.push_back(parse_expr(0));
        }

        return make<IdentNode>(alloc, 1, Type::EQUAL, std::move(left_side), std::move(right_side)); 
    }

    if (left_side.size() == 1 && 
       (left_side.front()->getName() == "FunctionCallNode" || 
        left_side.front()->getName() == "MethodCallNode"))
        return left_side.front();

    throwError(Type::IDENT);
    return nullptr;
}

Node* Parser::parse_local() {
    advance();

    [[maybe_unused]] 
    bool isClose = false;

    std::vector<bool> const_vect;
    std::vector<Node*, ArenaAllocator<Node*>> left_side(alloc);
    std::vector<Node*, ArenaAllocator<Node*>> right_side(alloc);

    if (!check(Type::IDENT)) {
        if (check(Type::KW_FUNCTION)) {
            return parse_function(true);
        }
        else throwError(Type::KW_FUNCTION);
    }

    // better use VariableNode{.name = peek()} instead of nud()
    auto left = nud();
    left_side.push_back(left);

    if (check(Type::LESS)) {
        advance();
        Type attr = peek().type;

        if (attr == Type::KW_CONST) {
            advance();
            const_vect.push_back(true);
        }
        else if (attr == Type::KW_CLOSE) { isClose = true; advance(); }
        else throwError(Type::KW_CONST);

        expect(Type::GREATER);
    }
    else const_vect.push_back(false); 

    while (check(Type::COMMA)) {
        if (check(Type::COMMA)) advance();

        if (check(Type::LESS)) {
            advance();
            Type attr = peek().type;

            if (attr == Type::KW_CONST) {
                advance();
                const_vect.push_back(true);
            }
            else if (attr == Type::KW_CLOSE) { isClose = true; advance(); }
            else throwError(Type::KW_CONST);

            expect(Type::GREATER);
        }
        else const_vect.push_back(false);

        left_side.push_back(nud());
    }

    if (check(Type::EQUAL)) {
        advance();

        auto right = parse_expr(0);
        right_side.push_back(right);

        while (check(Type::COMMA)) {
            if (check(Type::COMMA)) advance();

            right_side.push_back(parse_expr(0));
        }
    }

    return make<MultipleVariableNode>(
        alloc, 1,
        std::move(const_vect),
        std::move(left_side),
        std::move(right_side)
    );
}

Node* Parser::parse_do() {
    advance();

    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block();
    expect(Type::KW_END);

    return make<DoNode>(alloc, 1, std::move(body));
}

Node* Parser::parse_while() {
    advance();

    Node* condition = parse_expr(0); 

    if (!check(Type::KW_DO)) throwError(Type::KW_DO);
    advance();
    
    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block();
    expect(Type::KW_END);

    return make<WhileNode>(alloc, 1, 
        condition,
        std::move(body) 
    );
}

Node* Parser::parse_for() {
    advance();

    Node* start;
    Node* finish;
    Node* step = nullptr;

    if (!check(Type::IDENT)) { throwError(Type::IDENT); }
    Token var = peek();

    advance();

    if (check(Type::COMMA) || check(Type::KW_IN)) {
        std::vector<Node*, ArenaAllocator<Node*>> keyArgs(alloc);
        keyArgs.push_back(make<VariableNode>(alloc, 1, var));

        while (check(Type::COMMA)) {
            advance();
            keyArgs.push_back(make<VariableNode>(alloc, 1, advance()));
        }

        expect(Type::KW_IN);

        Node* iter_fn = parse_ident();
        if (!(iter_fn->getName() == "FunctionCallNode")) throwError(Type::CALLEDFUNCTION);

        expect(Type::KW_DO);

        std::vector<Node*, ArenaAllocator<Node*>> body = parse_block();
        expect(Type::KW_END);

        return make<GenericForNode>(alloc, 1, 
            std::move(keyArgs), iter_fn, std::move(body)
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

    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block();
    expect(Type::KW_END);

    return make<NumericForNode>(alloc, 1, 
        std::move(var), start,
        finish, step, std::move(body) 
    );
}

Node* Parser::parse_if() {
    advance();

    Node* condition = parse_expr(0); 

    if (check(Type::KW_THEN)) {
        advance();
    } else throwError(Type::KW_THEN);

    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block(); 

    std::vector<Node*, ArenaAllocator<Node*>> elseifs(alloc);
    while (peek().type == Type::KW_ELSEIF) {
        elseifs.push_back(parse_elseif());
    }

    std::vector<Node*, ArenaAllocator<Node*>> elseBody(alloc);
    if (peek().type == Type::KW_ELSE) {
        advance();
        elseBody = parse_block();
    }

    expect(Type::KW_END);

    return make<IfNode>(alloc, 1, 
        condition,
        std::move(body), 
        std::move(elseifs),
        std::move(elseBody)
    );
}

Node* Parser::parse_elseif() {
    advance();

    Node* condition = parse_expr(0);

    expect(Type::KW_THEN);

    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block(); 

    return make<ElseIfNode>(alloc, 1, 
        condition,
        std::move(body)
    );
}

Node* Parser::parse_repeat() {
    advance();
    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block();

    expect(Type::KW_UNTIL);

    Node* condition = parse_expr(0);

    return make<RepeatUntilNode>(alloc, 1, 
        condition,
        std::move(body)
    );
}

Node* Parser::parse_function(bool isLocal) {
    advance();
    
    if (!check(Type::IDENT)) {
        throwError(Type::IDENT);
    }

    Token funcName = advance();

    std::vector<Node*, ArenaAllocator<Node*>> Args(alloc);

    if (check(Type::L_PAREN)) {
        advance();

        while (!check(Type::R_PAREN)) {
            Args.push_back(parse_expr(0));

            if (check(Type::COMMA)) advance();
        }
        expect(Type::R_PAREN);
    }
    else {
        if (check(Type::DOT) || check(Type::COLON)) {
            advance();
            return parse_method(std::move(funcName), isLocal); 
        }
        else throwError(Type::COLON);
    } 
    
    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block(); 
    expect(Type::KW_END);

    return make<FunctionNode>(alloc, 1, 
        std::move(funcName),
        isLocal,
        std::move(Args),
        std::move(body)
    );
}

Node* Parser::parse_method(Token className, bool isLocal) {
    Token methodName = advance();
    std::vector<Node*, ArenaAllocator<Node*>> args(alloc);

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
    
    std::vector<Node*, ArenaAllocator<Node*>> body = parse_block(); 
    expect(Type::KW_END);

    return make<MethodNode>(alloc, 1, 
        std::move(methodName),
        std::move(className),
        isLocal,
        std::move(args),
        std::move(body)
    );
}

Node* Parser::parse_return() {
    advance();
    std::vector<Node*, ArenaAllocator<Node*>> args(alloc);

    while (!endblock()) {
        if (check(Type::KW_FUNCTION)) {
            advance();
            expect(Type::L_PAREN);

            std::vector<Node*, ArenaAllocator<Node*>> innerArgs(alloc);

            while (!check(Type::R_PAREN)) {
                innerArgs.push_back(parse_expr(0));

                if (!check(Type::COMMA) && !check(Type::R_PAREN)) { throwError(Type::R_PAREN); }
                if (check(Type::COMMA)) advance();
            }

            expect(Type::R_PAREN);

            std::vector<Node*, ArenaAllocator<Node*>> body = parse_block();
            expect(Type::KW_END);

            return make<AnonFunction>(alloc, 1, 
                std::move(innerArgs), std::move(body)
            );
        }

        args.push_back(parse_expr(0));

        if (check(Type::COMMA)) advance();
        else break;
    }

    return make<ReturnNode>(alloc, 1, std::move(args));
}

std::vector<Node*, ArenaAllocator<Node*>> Parser::parse_block() {
    std::vector<Node*, ArenaAllocator<Node*>> block(alloc);

    while (!endblock() && !check(Type::END_OF_FILE)) {
        Node* statement = parse_stat(); 
        if (statement) {
            block.push_back(statement); 
        }
    }

    return block;
}

Node* Parser::parse_stat() {
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
        case Type::KW_OR:          return 10;
        case Type::KW_AND:         return 20;
        case Type::VERTICAL_BAR:  return 30;
        case Type::TILDE:          return 30;
        case Type::AMPERSAND:     return 30;
        case Type::L_SHIFT:        return 30;
        case Type::R_SHIFT:        return 30;
        case Type::EQUAL_EQUAL:   return 40;
        case Type::NOT_EQUAL:     return 40;
        case Type::GREATER:        return 50;
        case Type::GREATER_EQUAL: return 50;
        case Type::LESS:           return 50;
        case Type::LESS_EQUAL:    return 50;
        case Type::CONCAT:         return 60;
        case Type::PLUS:           return 70;
        case Type::MINUS:          return 70;
        case Type::PERCENT:        return 80;
        case Type::DOUBLE_SLASH:  return 80;
        case Type::SLASH:          return 80;
        case Type::CARET:          return 90;
        case Type::STAR:
            if (checkNext(Type::STAR)) return 90;
            return 80;
        case Type::NOT:            return 100;
        case Type::DOT:            return 110;
        case Type::L_PAREN:        return 110;
        case Type::L_BRACKET:     return 110;
        case Type::COLON:          return 110;
        default: return 0;
    }
}

Node* Parser::nud() {
    if (check(Type::LIT_INT) || check(Type::LIT_FLOAT) || check(Type::LIT_STRING)
        || check(Type::KW_TRUE) || check(Type::KW_FALSE) || check(Type::LIT_LONG_STRING)
        || check(Type::LIT_HEX) || check(Type::LIT_CHAR) || check(Type::ELLIPSIS) 
        || check(Type::KW_NIL)
    ) {
        Token value = peek();
        advance();

        return make<BasicDataNode>(alloc, 1, std::move(value));
    }
    else if (std::ranges::find(UnaryOpSet, peek().type) != UnaryOpSet.end()) {
        Token op = advance();

        auto val = parse_expr(90);

        return make<UnaryOpNode>(alloc, 1, std::move(op), val);
    }
    else if (check(Type::L_PAREN)) {
        advance();

        auto inner_expr = parse_expr(0);
        expect(Type::R_PAREN);

        return inner_expr;
    }
    else if (check(Type::L_BRACE)) {
        advance();
        std::vector<Node*, ArenaAllocator<Node*>> elements(alloc);

        while (!check(Type::R_BRACE)) {
            if (check(Type::L_BRACKET)) {
                advance();

                auto index_expr = parse_expr(0);
                auto index = make<IndexNode>(alloc, 1, index_expr);

                expect(Type::R_BRACKET);

                if (check(Type::EQUAL)) {
                    advance();

                    auto value = parse_expr(0);

                    elements.push_back(make<TableFieldNode>(alloc, 1, 
                        index, value));
                }
                else elements.push_back(index);
            }
            else { 
                auto key = parse_expr(0);

                if (check(Type::EQUAL)) {
                    advance();
                    auto value = parse_expr(0);

                    elements.push_back(make<TableFieldNode>(alloc, 1, 
                        key, value));
                }
                else elements.push_back(key);
            }

            if (check(Type::COMMA)) advance();
        } 
        expect(Type::R_BRACE);

        return make<ArrayNode>(alloc, 1, std::move(elements));
    }
    else if (check(Type::IDENT)) {
        Token value = peek();
        advance();

        return make<VariableNode>(alloc, 1, std::move(value));
    }
    else if (check(Type::KW_FUNCTION)) {
        advance();
        expect(Type::L_PAREN);

        std::vector<Node*, ArenaAllocator<Node*>> innerArgs(alloc);

        while (!check(Type::R_PAREN)) {
            innerArgs.push_back(parse_expr(0));

            if (!check(Type::COMMA) && !check(Type::R_PAREN)) {
                throwError(Type::R_PAREN); 
            }

            if (check(Type::COMMA)) advance();
        }

        expect(Type::R_PAREN);

        std::vector<Node*, ArenaAllocator<Node*>> body = parse_block();
        expect(Type::KW_END);

        return make<AnonFunction>(alloc, 1, 
            std::move(innerArgs), std::move(body)
        );
    }

    throwError(Type::IDENT);
    return nullptr;
}

Node* Parser::parse_expr(int min_lbp) {
    Node* left = nud(); 
    
    while (true) {
        int lbp = get_lbp();
        if (lbp <= min_lbp) break;

        Type op = peek().type;

        if (op == Type::STAR && peekNext().type == Type::STAR) {
            advance(); advance();

            Node* right = parse_expr(lbp - 1); 
            left = make<BinaryOpNode>(alloc, 1, 
                op,
                left,
                right
            );

            continue;
        }

        if (op == Type::CONCAT) {
            advance();

            Node* right = parse_expr(lbp - 1); 

            left = make<BinaryOpNode>(alloc, 1, 
                op,
                left,
                right
            );
            continue;
        }

        if (op == Type::DOT) {
            advance();

            if (!check(Type::IDENT)) throwError(Type::IDENT);
            Token q = advance();

            left = make<MemberAccessNode>(alloc, 1, left, std::move(q));
            continue;
        }

        if (op == Type::L_PAREN) {
            advance(); 

            std::vector<Node*, ArenaAllocator<Node*>> args(alloc);

            while (!check(Type::R_PAREN)) {
                args.push_back(parse_expr(0));

                if (check(Type::COMMA)) advance();
            }

            expect(Type::R_PAREN); 

            left = make<FunctionCallNode>(alloc, 1, 
                left,
                std::move(args)
            );
            continue;
        }

        if (op == Type::COLON) {
            advance();
            Token method_name = advance();

            std::vector<Node*, ArenaAllocator<Node*>> args(alloc);
            expect(Type::L_PAREN);

            while (!check(Type::R_PAREN)) {
                args.push_back(parse_expr(0));
                if (check(Type::COMMA)) advance();
            }

            expect(Type::R_PAREN); 

            left = make<MethodCallNode>(alloc, 1, 
                std::move(method_name), 
                left,
                std::move(args)
            );
            continue;
        }

        if (op == Type::L_BRACKET) {
            advance();

            auto index_expr = parse_expr(lbp);
            expect(Type::R_BRACKET);

            left = make<ExprWithIndexNode>(alloc, 1, 
                left,
                index_expr
            );
            continue;
        }

        if (op == Type::KW_AND) {
            advance();
            auto right = parse_expr(lbp);

            left = make<AndTernaryNode>(alloc, 1, 
                left,
                right
            );
            continue;
        }

        if (op == Type::KW_OR) {
            advance();
            auto right = parse_expr(lbp);

            left = make<OrTernaryNode>(alloc, 1, 
                left,
                right
            );
            continue;
        }

        if (std::ranges::find(BitwiseOpSet, op) != BitwiseOpSet.end()) {
            advance();
            auto right = parse_expr(lbp);

            left = make<BitwiseNode>(alloc, 1, 
                op,
                left,
                right
            );
            continue;
        }

        if (lbp > 0) {
            advance();
            Node* right = parse_expr(lbp);

            left = make<BinaryOpNode>(alloc, 1, 
                op,
                left,
                right
            );
        }
        else break;
    }

    return left;
}
