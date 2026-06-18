// need to handle: 
// ungrade parse_local so it can parse local a, b = 1, 3
// option 1: create MultipleVariableNode, which will contain map of variables and values

#include <parser.hpp>
#include <format>

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

  std::unordered_map<Type, std::string>::iterator exp = TokenToStrMAP.find(expected);

  if (exp != TokenToStrMAP.end()) expectedStr = exp->second;
  else throw std::runtime_error(
      std::format("internal compiler error: type not found in tokentostrmap in line {}, col {}",
                                            positionY, positionX)
  );

  std::unordered_map<Type, std::string>::iterator act = TokenToStrMAP.find(peek().type);

  if (act != TokenToStrMAP.end()) actualStr = act->second;
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

std::unique_ptr<Node> Parser::parse_local() {
  advance();

  if (!check(Type::IDENT)) {
    if (check(Type::KW_FUNCTION)) {
     return parse_function(true);
    }
    else throwError(Type::KW_FUNCTION);
  }

  Token name = advance();
  std::unique_ptr<Node> expr = nullptr;

  if (check(Type::L_BRACKET)) {
    advance();
    std::vector<std::unique_ptr<Node>> array;
    std::unique_ptr<Node> element;

    while (!check(Type::R_BRACKET)) { 
      array.push_back(parse_expr(0)); 

      if (check(Type::COMMA)) advance();
    }

    expect(Type::R_BRACKET);

    return std::make_unique<LocalNode>(
      std::move(name),
      nullptr,
      std::move(array)
    );
  }

  if (check(Type::EQUAL)) {
    advance();
    expr = parse_expr(0);
  }

  return std::make_unique<LocalNode>(
    std::move(name),
    std::move(expr),
    std::vector<std::unique_ptr<Node>>{}
  );
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

  Token var;
  std::unique_ptr<Node> start;
  std::unique_ptr<Node> finish;
  std::unique_ptr<Node> step;

  if (!check(Type::IDENT)) { throwError(Type::IDENT); }
  var = peek();

  advance();

  if (check(Type::COMMA)) {
    std::vector<Token> keyArgs;
    keyArgs.push_back(var);

    while (!check(Type::KW_IN)) {
      if (check(Type::COMMA)) { advance(); }

      keyArgs.push_back(peek());
    }

    expect(Type::KW_IN);

    // checking if its even a function
    std::unique_ptr<Node> iter_fn = parse_ident();
    if (!(iter_fn->getName() == "CalledFunctionNode")) throwError(Type::CALLEDFUNCTION);

    expect(Type::KW_DO);

    return std::make_unique<GenericForNode>(
      std::move(keyArgs), std::move(iter_fn)  
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

  if (check(Type::KW_THEN)) {
    advance();
  } else throwError(Type::KW_THEN);

  std::vector<std::unique_ptr<Node>> body = parse_block(); 

  return std::make_unique<ElseIfNode>(
    std::move(condition),
    std::move(body)
  );
}

std::unique_ptr<Node> Parser::parse_function(bool isLocal) {
  advance();
  
  if (!check(Type::IDENT)) {
    throwError(Type::IDENT);
  }
  Token funcName = peek();
  advance();

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
    if (check(Type::COLON_COLON)) {
      advance();
      Token className = peek();
      return parse_method(className, isLocal);
    }
    else throwError(Type::COLON_COLON);
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
  advance();
  
  if (!check(Type::IDENT)) {
    throwError(Type::IDENT);
  }

  Token methodName = peek();
  advance();

  std::vector<std::unique_ptr<Node>> args;
  if (check(Type::L_PAREN)) {
    advance();

    while (!check(Type::R_PAREN)) {
      if (!check(Type::IDENT)) {
        throwError(Type::IDENT);
      }

      args.push_back(parse_expr(0));

      if (!check(Type::COMMA) && !check(Type::R_PAREN)) { throwError(Type::R_PAREN); }
      if (check(Type::COMMA)) advance();
    }

    advance();
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

std::unique_ptr<Node> Parser::parse_ident() {
  Token value = peek();
  std::vector<std::unique_ptr<Node>> argsVect;

  advance();

  if (check(Type::L_PAREN)) {
    advance();

    while (!check(Type::R_PAREN)) {
      argsVect.push_back(parse_expr(0));

      if (check(Type::COMMA)) advance();
    }

    expect(Type::R_PAREN);

    return std::make_unique<CalledFunctionNode>(
      std::move(value),
      std::move(argsVect)
    );
  }
  else if (check(Type::EQUAL)) {
    advance();
    std::unique_ptr<Node> right = parse_expr(0);

    return std::make_unique<BinaryOpNode>(
      Type::EQUAL,
      std::make_unique<VariableNode>(std::move(value)),
      std::move(right)
    );
  }
  else if (check(Type::DOT)) {
    advance();

    Token val = peek();
    std::vector<std::unique_ptr<Node>> argsVect;

    advance();

    if (check(Type::L_PAREN)) {
      advance();

      while (!check(Type::R_PAREN)) {
        argsVect.push_back(parse_expr(0));

        if (check(Type::COMMA)) advance();
      }

      expect(Type::R_PAREN);

      return std::make_unique<CalledMethodNode>(
        std::move(val),
        std::move(value),
        std::move(argsVect)
      );
    }
  }

  throwError(Type::EQUAL);
}

std::unique_ptr<Node> Parser::parse_return() {
  advance();
  std::vector<std::unique_ptr<Node>> args;

  while (!endblock()) {
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
    case Type::IDENT:
      return parse_ident();
    case Type::KW_RETURN:
      return parse_return();
    default:
      return parse_expr(0); 
  } 
}

int Parser::get_lbp() {
  switch (peek().type) {
    case Type::EQUAL:         return 10;
    case Type::VERTICAL_BAR:  return 20;
    case Type::AMPERSAND:     return 30;
    case Type::EQUAL_EQUAL:   return 40;
    case Type::NOT_EQUAL:     return 40;
    case Type::GREATER:       return 50;
    case Type::GREATER_EQUAL: return 50;
    case Type::LESS:          return 50;
    case Type::LESS_EQUAL:    return 50;
    case Type::CONCAT:        return 55;
    case Type::PLUS:          return 60;
    case Type::MINUS:         return 60;
    case Type::SLASH:         return 70;
    case Type::STAR:
      if (peekNext().type == Type::STAR) return 80;
      return 70;

    default: return 0;
  }
}

std::unique_ptr<Node> Parser::nud() {
  if (check(Type::LIT_INT) || check(Type::LIT_FLOAT) || check(Type::LIT_STRING)
      || check(Type::KW_TRUE) || check(Type::KW_FALSE))
  {
    Token value = peek();
    advance();

    return std::make_unique<BasicDataNode>(std::move(value));
  }
  if (check(Type::IDENT)) {
    Token value = peek();
    advance();

    if (check(Type::L_PAREN)) {
      advance();

      std::vector<std::unique_ptr<Node>> args;
      
      while (!check(Type::R_PAREN)) {
        args.push_back(parse_expr(0));

        if (check(Type::COMMA)) advance();
      }

      expect(Type::R_PAREN);
      
      return std::make_unique<CalledFunctionNode>(std::move(value), std::move(args));
    }
    else if (check(Type::DOT)) {
      advance();
      if (!check(Type::IDENT)) throwError(Type::IDENT);
      Token qualifier = advance();

      if (check(Type::L_PAREN)) {
        advance();

        std::vector<std::unique_ptr<Node>> args;
        
        while (!check(Type::R_PAREN)) {
          args.push_back(parse_expr(0));

          if (check(Type::COMMA)) advance();
        }

        expect(Type::R_PAREN);
        
        return std::make_unique<CalledMethodNode>(std::move(value), std::move(qualifier), std::move(args));
      }
    }
    if (UnaryOpSet.contains(peek().type)) {
      Token op = peek();
      Token val = peek();

      advance();

      return std::make_unique<UnaryOpNode>(std::move(op), std::move(val));
    }
    else return std::make_unique<BasicDataNode>(value);
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

    advance();

    std::unique_ptr<Node> right; 

    if (op == Type::EQUAL) {
      right = parse_expr(lbp-1);
    }
    else right = parse_expr(lbp);

    left = std::make_unique<BinaryOpNode>(
      op,
      std::move(left),
      std::move(right)
    );
  }

  return left;
}
