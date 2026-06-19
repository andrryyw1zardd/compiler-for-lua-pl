#include <lexer.hpp>
#include <stdbool.h>
#include <memory>
#include <unordered_set>

#ifndef PARSER_HPP
#define PARSER_HPP

struct Node {
  Vect2 position;

  virtual ~Node() = default;
  virtual std::string getName() = 0;
};

struct UnaryOpNode : Node {
  Token op;
  std::unique_ptr<Node> value;

  UnaryOpNode(Token o, std::unique_ptr<Node> v) : op(std::move(o)), value(std::move(v)) {}

  std::string getName() {
    return "UnaryOpNode";
  }
};

struct BinaryOpNode : Node {
  Type op;

  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;

  BinaryOpNode(Type op, std::unique_ptr<Node> l, std::unique_ptr<Node> r) 
    : op(op), left(std::move(l)), right(std::move(r)) { }

  std::string getName() override {
    return "BinaryOpNode";
  }
};

struct MultipleVariableNode : Node {
  Type op;
  std::vector<std::unique_ptr<Node>> left_side;
  std::vector<std::unique_ptr<Node>> right_side;

  MultipleVariableNode(Type op, std::vector<std::unique_ptr<Node>> ls, std::vector<std::unique_ptr<Node>> rs)
    : op(std::move(op)), left_side(std::move(ls)), right_side(std::move(rs)) { }

  std::string getName() {
    return "MultipleVariableNode";
  }
};

struct BasicDataNode : Node {
  Token value;

  BasicDataNode(Token v) : value(std::move(v)) { }

  std::string getName() override {
    return "BasicDataNode";
  }
};

struct IfNode : Node {
  std::unique_ptr<Node> condition;
  std::vector<std::unique_ptr<Node>> body;
  std::vector<std::unique_ptr<Node>> elseifs;
  std::vector<std::unique_ptr<Node>> elseBody;

  IfNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> b,
         std::vector<std::unique_ptr<Node>> ei, std::vector<std::unique_ptr<Node>> eb)
    : condition(std::move(c)), body(std::move(b)), elseifs(std::move(ei)), elseBody(std::move(eb)){ };

  std::string getName() override {
    return "IfNode";
  }
};

struct ElseIfNode : Node {
  std::unique_ptr<Node> condition;
  std::vector<std::unique_ptr<Node>> body;

  ElseIfNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> b) 
    : condition(std::move(c)), body(std::move(b)) { };

  std::string getName() override {
    return "ElseIfNode";
  }
};

struct WhileNode : Node {
  std::unique_ptr<Node> condition;
  std::vector<std::unique_ptr<Node>> body;

  WhileNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> b) 
    : condition(std::move(c)), body(std::move(b)) { } 

  std::string getName() override {
    return "WhileNode";
  }
};

struct NumericForNode : Node {
  Token var;
  std::unique_ptr<Node> start;
  std::unique_ptr<Node> finish;
  std::unique_ptr<Node> step;
  std::vector<std::unique_ptr<Node>> body;

  NumericForNode(Token v, std::unique_ptr<Node> s, std::unique_ptr<Node> f,
          std::unique_ptr<Node> st, std::vector<std::unique_ptr<Node>> b) 
  : var(std::move(v)), start(std::move(s)), finish(std::move(f)), step(std::move(st)), body(std::move(b)) { }

  std::string getName() override {
    return "NumericForNode";
  }
};

struct GenericForNode : Node {
  std::vector<Token> keyArgs;
  std::unique_ptr<Node> fn;

  GenericForNode(std::vector<Token> ka, std::unique_ptr<Node> f) : keyArgs(std::move(ka)), fn(std::move(f)) {}

  std::string getName() override {
    return "GenericForNode";
  }
};

struct FunctionNode : Node {
  Token value;
  bool isLocal = false;

  std::vector<std::unique_ptr<Node>> args;
  std::vector<std::unique_ptr<Node>> body;

  FunctionNode(Token v, bool isL, std::vector<std::unique_ptr<Node>> a, std::vector<std::unique_ptr<Node>> b)
  : value(std::move(v)), isLocal(isL), args(std::move(a)), body(std::move(b)) {}
  
  std::string getName() override {
    return "FunctionNode";
  }
};

struct MethodNode : Node {
  Token value;
  Token className;
  bool isLocal = false;
  
  std::vector<std::unique_ptr<Node>> args;
  std::vector<std::unique_ptr<Node>> body;

  MethodNode(Token v, Token cn, bool isL, std::vector<std::unique_ptr<Node>> a, std::vector<std::unique_ptr<Node>> b)
  : value(std::move(v)), className(std::move(cn)), isLocal(isL), args(std::move(a)), body(std::move(b)) {}

  std::string getName() override {
    return "MethodNode";
  }
};

struct CallNode : Node {
  std::unique_ptr<Node> callee;
  std::vector<std::unique_ptr<Node>> args;

  CallNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> a) 
    : callee(std::move(c)), args(std::move(a)) { }

  std::string getName() override {
    return "CallNode";
  }
};

struct MemberAccessNode : Node {
  std::unique_ptr<Node> value;
  Token qualifier;

  MemberAccessNode(std::unique_ptr<Node> v, Token q) 
    : value(std::move(v)), qualifier(std::move(q)) { }

  std::string getName() override {
    return "MemberAccessNode";
  }
};


struct VariableNode : Node {
  Token value;

  VariableNode(Token v) : value(v) { }

  std::string getName() override {
    return "VariableNode";
  }
};

struct LocalNode : Node {
  Token value;

  std::unique_ptr<Node> right;
  std::vector<std::unique_ptr<Node>> arrayElements;

  LocalNode(Token v, std::unique_ptr<Node> r, std::vector<std::unique_ptr<Node>> ae)
    : value(std::move(v)), right(std::move(r)), arrayElements(std::move(ae)) { }  

  std::string getName() override {
    return "LocalNode";
  }
};

struct ReturnNode : Node {
  std::vector<std::unique_ptr<Node>> args;

  ReturnNode(std::vector<std::unique_ptr<Node>> a) : args(std::move(a)) { }

  std::string getName() override {
    return "ReturnNode";
  }
};

class Parser {
private:
  std::vector<Token> listOfTokens;
  unsigned long long index = 0;
  Type endblockArray[5] = {Type::KW_END, Type::KW_ELSE,
                          Type::KW_ELSEIF, Type::KW_UNTIL,
                          Type::END_OF_FILE};

  std::unordered_map<Type, std::string> TokenToStrMAP = {
    {Type::KW_IF,          "'if'"},
    {Type::KW_ELSEIF,      "'else if'"},
    {Type::KW_ELSE,        "'else'"},
    {Type::KW_LOCAL,       "'local'"},
    {Type::KW_THEN,        "'then'"},
    {Type::KW_END,         "'end'"},
    {Type::KW_FUNCTION,    "'function'"},
    {Type::KW_RETURN,      "'return'"},
    {Type::KW_WHILE,       "'while'"},
    {Type::KW_FOR,         "'for'"},
    {Type::KW_DO,          "'do'"},
    {Type::KW_REPEAT,      "'repeat'"},
    {Type::KW_UNTIL,       "'until'"},
    {Type::KW_NIL,         "'nil'"},
    {Type::KW_IN,          "'in'"},
    {Type::KW_TRUE,        "'true'"},
    {Type::KW_FALSE,       "'false'"},
    {Type::KW_AND,         "'and'"},
    {Type::KW_OR,          "'or'"},
    {Type::KW_NOT,         "'not'"},
    {Type::IDENT,          "'identificator'"},
    {Type::CALLEDFUNCTION, "'function invocation'"},
    {Type::PLUS,           "'+'"},
    {Type::MINUS,          "'-'"},
    {Type::STAR,           "'*'"},
    {Type::PERCENT,        "'%'"},
    {Type::CARET,          "'^'"},
    {Type::CONCAT,         "'..'"},
    {Type::DOT,            "'.'"},
    {Type::HASH,           "'#'"},
    {Type::L_PAREN,        "'('"},
    {Type::R_PAREN,        "')'"},
    {Type::L_BRACE,        "'{'"},
    {Type::R_BRACE,        "'}'"},
    {Type::L_BRACKET,      "'['"},
    {Type::R_BRACKET,      "']'"},
    {Type::COMMA,          "','"},
    {Type::SEMICOLON,      "';'"},
    {Type::COLON_COLON,    "':'"},
    {Type::EQUAL,          "'='"},
    {Type::EQUAL_EQUAL,    "'=='"},
    {Type::NOT_EQUAL,      "'!='"},
    {Type::LESS,           "'<'"},
    {Type::GREATER,        "'>'"},
    {Type::LESS_EQUAL,     "'<='"},
    {Type::GREATER_EQUAL,  "'>='"},
    {Type::SLASH,          "'/'"},
    {Type::AMPERSAND,      "'&'"},
    {Type::VERTICAL_BAR,   "'|'"},
    {Type::LIT_INT,        "'integer literal'"},
    {Type::LIT_STRING,     "'string literal'"},
    {Type::LIT_FLOAT,      "'float literal'"},
    {Type::END_OF_FILE,    "'end of file'"},
  };

  std::unordered_set<Type> UnaryOpSet = {
    Type::MINUS, Type::PLUS, Type::HASH, Type::NOT
  };

  Token peek();
  Token peekNext();
  Token advance();
  bool  check(Type type);
  bool checkNext(Type type); 
  void  expect(Type type);
  [[noreturn]] void throwError(Type expected);
   
  bool endblock();
  int  get_lbp(); 
  std::unique_ptr<Node> nud(); 
  std::unique_ptr<Node> parse_expr(int min_lbp);

  std::unique_ptr<Node> parse_if();
  std::unique_ptr<Node> parse_elseif();
  std::unique_ptr<Node> parse_while();
  std::unique_ptr<Node> parse_for();
  std::unique_ptr<Node> parse_return();
  std::unique_ptr<Node> parse_local();
  std::unique_ptr<Node> parse_function(bool isLocal);
  std::unique_ptr<Node> parse_ident();
  std::unique_ptr<Node> parse_method(Token className, bool isLocal); 

public:
  std::vector<std::unique_ptr<Node>> parse_block(); 
  std::unique_ptr<Node> parse_stat();

  Parser(std::vector<Token> VectorOfTokens) : listOfTokens(VectorOfTokens) {}
};

#endif
