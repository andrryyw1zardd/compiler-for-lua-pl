#include <lexer/lexer.hpp>
#include <stdbool.h>
#include <memory>
#include <unordered_set>
#include <string_view>

#ifndef PARSER_HPP
#define PARSER_HPP

struct Node {
  Vect2 position;

  virtual ~Node() = default;
  virtual std::string_view getName() const = 0;
};

struct UnaryOpNode : Node {
  Token op;
  std::unique_ptr<Node> value;

  UnaryOpNode(Token o, std::unique_ptr<Node> v)
  : op(std::move(o)), value(std::move(v)) {}

  std::string_view getName() const override {
    return "UnaryOpNode";
  }
};

struct BinaryOpNode : Node {
  Type op;

  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;

  BinaryOpNode(Type op, std::unique_ptr<Node> l, std::unique_ptr<Node> r) 
    : op(op), left(std::move(l)), right(std::move(r)) { }

  std::string_view getName() const override {
    return "BinaryOpNode";
  }
};

struct MemberAccessNode : Node {
  std::unique_ptr<Node> value;
  Token qualifier;

  MemberAccessNode(std::unique_ptr<Node> v, Token q) 
    : value(std::move(v)), qualifier(std::move(q)) { }

  std::string_view getName() const override {
    return "MemberAccessNode";
  }
};

struct BasicDataNode : Node {
  Token value;

  BasicDataNode(Token v) : value(std::move(v)) { }

  std::string_view getName() const override {
    return "BasicDataNode";
  }
};

struct VariableNode : Node {
  Token value;

  VariableNode(Token v) : value(std::move(v)) { }

  std::string_view getName() const override {
    return "VariableNode";
  }
};

struct DefineVariableNode : Node {
  Token value;

  std::unique_ptr<Node> right;

  DefineVariableNode(Token v, std::unique_ptr<Node> r)
    : value(std::move(v)), right(std::move(r)) { }  

  std::string_view getName() const override {
    return "DefineVariableNode";
  }
};

struct MultipleVariableNode : Node {
  Type op;
  std::vector<std::unique_ptr<Node>> left_side;
  std::vector<std::unique_ptr<Node>> right_side;

  MultipleVariableNode(Type op, std::vector<std::unique_ptr<Node>> ls, std::vector<std::unique_ptr<Node>> rs)
    : op(std::move(op)), left_side(std::move(ls)), right_side(std::move(rs)) { }

  std::string_view getName() const override {
    return "MultipleVariableNode";
  }
};

struct ArrayNode : Node {
  std::vector<std::unique_ptr<Node>> elements;

  ArrayNode(std::vector<std::unique_ptr<Node>> e) : elements(std::move(e)) { }

  std::string_view getName() const override {
    return "ArrayNode";
  }
};

struct TableFieldNode : Node {
  Token key;
  std::unique_ptr<Node> value;

  TableFieldNode(Token k, std::unique_ptr<Node> v) : key(std::move(k)), value(std::move(v)) {}

  std::string_view getName() const override {
    return "TableFieldNode"; 
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

  std::string_view getName() const override {
    return "IfNode";
  }
};

struct ElseIfNode : Node {
  std::unique_ptr<Node> condition;
  std::vector<std::unique_ptr<Node>> body;

  ElseIfNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> b) 
    : condition(std::move(c)), body(std::move(b)) { };

  std::string_view getName() const override {
    return "ElseIfNode";
  }
};

struct WhileNode : Node {
  std::unique_ptr<Node> condition;
  std::vector<std::unique_ptr<Node>> body;

  WhileNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> b) 
    : condition(std::move(c)), body(std::move(b)) { } 

  std::string_view getName() const override {
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

  std::string_view getName() const override {
    return "NumericForNode";
  }
};

struct GenericForNode : Node {
  std::vector<Token> keyArgs;
  std::unique_ptr<Node> fn;
  std::vector<std::unique_ptr<Node>> body;


  GenericForNode(std::vector<Token> ka, std::unique_ptr<Node> f, std::vector<std::unique_ptr<Node>> b) 
  : keyArgs(std::move(ka)), fn(std::move(f)), body(std::move(b)) {}

  std::string_view getName() const override {
    return "GenericForNode";
  }
};

struct RepeatUntilNode : Node {
  std::unique_ptr<Node> condition;
  std::vector<std::unique_ptr<Node>> body;

  RepeatUntilNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> b) 
    : condition(std::move(c)), body(std::move(b)) { } 

  std::string_view getName() const override {
    return "RepeatUntilNode";
  }
};

struct FunctionNode : Node {
  Token value;
  bool isLocal = false;

  std::vector<std::unique_ptr<Node>> args;
  std::vector<std::unique_ptr<Node>> body;

  FunctionNode(Token v, bool isL, std::vector<std::unique_ptr<Node>> a, std::vector<std::unique_ptr<Node>> b)
  : value(std::move(v)), isLocal(isL), args(std::move(a)), body(std::move(b)) {}
  
  std::string_view getName() const override {
    return "FunctionNode";
  }
};

struct AnonFunction : Node {
  std::vector<std::unique_ptr<Node>> args;
  std::vector<std::unique_ptr<Node>> body;

  AnonFunction(std::vector<std::unique_ptr<Node>> a, std::vector<std::unique_ptr<Node>> b) 
  : args(std::move(a)), body(std::move(b)) { }

  std::string_view getName() const override {
    return "AnonFunction";
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

  std::string_view getName() const override {
    return "MethodNode";
  }
};

struct FunctionCallNode : Node {
  std::unique_ptr<Node> callee;
  std::vector<std::unique_ptr<Node>> args;

  FunctionCallNode(std::unique_ptr<Node> c, std::vector<std::unique_ptr<Node>> a) 
    : callee(std::move(c)), args(std::move(a)) { }

  std::string_view getName() const override {
    return "FunctionCallNode";
  }
};

struct MethodCallNode : Node {
  Token method_name;
  std::unique_ptr<Node> object_name;
  std::vector<std::unique_ptr<Node>> args;

  MethodCallNode(Token m, std::unique_ptr<Node> o, std::vector<std::unique_ptr<Node>> a) 
  : method_name(std::move(m)), object_name(std::move(o)), args(std::move(a)) {}

  std::string_view getName() const override {
    return "MethodCallNode";
  }
};

struct ReturnNode : Node {
  std::vector<std::unique_ptr<Node>> args;

  ReturnNode(std::vector<std::unique_ptr<Node>> a) : args(std::move(a)) { }

  std::string_view getName() const override {
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
    {Type::COLON,          "':'"},
    {Type::COLON_COLON,    "'::'"},
    {Type::EQUAL,          "'='"},
    {Type::ELLIPSIS,       "'...'"},
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
    {Type::ERROR,          "'ERROR'"},
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
  std::unique_ptr<Node> parse_repeat();
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
