#ifndef PARSER_HPP
#define PARSER_HPP

#include <lexer/lexer.hpp>
#include <stdbool.h>
#include <string_view>
#include <array>
#include <vector>
#include "allocator/alloc.hpp"
#include "sema/sema.hpp"

struct Node {
    Vect2 position;

    virtual ~Node() = default;
    virtual std::string_view getName() const = 0;
    virtual void accept(Visitor& v) = 0;
};

struct UnaryOpNode : Node {
    Token op;
    Node* value;

    UnaryOpNode(Token o, Node* v)
    : op(std::move(o)), value(std::move(v)) {}

    std::string_view getName() const override {
        return "UnaryOpNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct BinaryOpNode : Node {
    Type op;

    Node* left;
    Node* right;

    BinaryOpNode(Type op, Node* l, Node* r) 
        : op(std::move(op)), left(std::move(l)), right(std::move(r)) { }

    std::string_view getName() const override {
        return "BinaryOpNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct BitwiseNode : Node {
    Type op;

    Node* left;
    Node* right;

    BitwiseNode(Type op, Node* l, Node* r) 
        : op(std::move(op)), left(std::move(l)), right(std::move(r)) { }

    std::string_view getName() const override {
        return "BitwiseNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct MemberAccessNode : Node {
    Node* value;
    Token qualifier;

    MemberAccessNode(Node* v, Token q) 
        : value(std::move(v)), qualifier(std::move(q)) { }

    std::string_view getName() const override {
        return "MemberAccessNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct BasicDataNode : Node {
    Token value;

    BasicDataNode(Token v) : value(std::move(v)) { }

    std::string_view getName() const override {
        return "BasicDataNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct VariableNode : Node {
    Token value;

    VariableNode(Token v) : value(std::move(v)) { }

    std::string_view getName() const override {
        return "VariableNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct VarWithAttributeNode : Node {
    Type type;
    Token value;
    Node* right;

    VarWithAttributeNode (Type t, Token v, Node* r)
        : type(std::move(t)), value(std::move(v)), right(std::move(r)) { }  

    std::string_view getName() const override {
        return "VarWithAttributeNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct DefineVariableNode : Node {
    Token value;
    Node* right;

    DefineVariableNode(Token v, Node* r)
        : value(std::move(v)), right(std::move(r)) { }  

    std::string_view getName() const override {
        return "DefineVariableNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct MultipleVariableNode : Node {
    Type op;
    std::vector<Node*, ArenaAllocator<Node*>> left_side;
    std::vector<Node*, ArenaAllocator<Node*>> right_side;

    MultipleVariableNode(Type op, std::vector<Node*, ArenaAllocator<Node*>> ls, std::vector<Node*, ArenaAllocator<Node*>> rs)
        : op(std::move(op)), left_side(std::move(ls)), right_side(std::move(rs)) { }

    std::string_view getName() const override {
        return "MultipleVariableNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct AndTernaryNode : Node {
    Node* left;
    Node* right;

    AndTernaryNode(Node* l, Node* r) 
    : left(std::move(l)), right(std::move(r)) { }

    std::string_view getName() const override {
        return "AndTernaryNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct OrTernaryNode : Node {
    Node* left;
    Node* right;

    OrTernaryNode(Node* l, Node* r) 
    : left(std::move(l)), right(std::move(r)) { }

    std::string_view getName() const override {
        return "OrTernaryNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct ArrayNode : Node {
    std::vector<Node*, ArenaAllocator<Node*>> elements;

    ArrayNode(std::vector<Node*, ArenaAllocator<Node*>> e) : elements(std::move(e)) { }

    std::string_view getName() const override {
        return "ArrayNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct ExprWithIndexNode : Node {
    Node* left;
    Node* index_expr;

    ExprWithIndexNode(Node* l, Node* ie) 
    : left(std::move(l)), index_expr(std::move(ie)) { }

    std::string_view getName() const override {
        return "ExprWithIndexNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct IndexNode : Node {
    Node* index_expr;

    IndexNode(Node* ie) : index_expr(std::move(ie)) { }

    std::string_view getName() const override {
        return "IndexNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct XORNode : Node {
    Node* left;
    Node* right;

    XORNode(Node* l, Node* r) 
    : left(std::move(l)), right(std::move(r)) { }

    std::string_view getName() const override {
        return "XORNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct TableFieldNode : Node {
    Node* key;
    Node* value;

    TableFieldNode(Node* k, Node* v)
    : key(std::move(k)), value(std::move(v)) { }

    std::string_view getName() const override {
        return "TableFieldNode"; 
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct IfNode : Node {
    Node* condition;
    std::vector<Node*, ArenaAllocator<Node*>> body;
    std::vector<Node*, ArenaAllocator<Node*>> elseifs;
    std::vector<Node*, ArenaAllocator<Node*>> elseBody;

    IfNode(Node* c, std::vector<Node*, ArenaAllocator<Node*>> b,
           std::vector<Node*, ArenaAllocator<Node*>> ei, std::vector<Node*, ArenaAllocator<Node*>> eb)
        : condition(std::move(c)), body(std::move(b)), elseifs(std::move(ei)), elseBody(std::move(eb)){ };

    std::string_view getName() const override {
        return "IfNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct ElseIfNode : Node {
    Node* condition;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    ElseIfNode(Node* c, std::vector<Node*, ArenaAllocator<Node*>> b) 
        : condition(std::move(c)), body(std::move(b)) { };

    std::string_view getName() const override {
        return "ElseIfNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct DoNode : Node {
    std::vector<Node*, ArenaAllocator<Node*>> body;

    DoNode(std::vector<Node*, ArenaAllocator<Node*>> b) : body(std::move(b)) { }

    std::string_view getName() const override {
        return "DoNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct WhileNode : Node {
    Node* condition;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    WhileNode(Node* c, std::vector<Node*, ArenaAllocator<Node*>> b) 
        : condition(std::move(c)), body(std::move(b)) { } 

    std::string_view getName() const override {
        return "WhileNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct NumericForNode : Node {
    Token var;
    Node* start;
    Node* finish;
    Node* step;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    NumericForNode(Token v, Node* s, Node* f,
          Node* st, std::vector<Node*, ArenaAllocator<Node*>> b) 
    : var(std::move(v)), start(std::move(s)), finish(std::move(f)), step(std::move(st)), body(std::move(b)) { }

    std::string_view getName() const override {
        return "NumericForNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct GenericForNode : Node {
    std::vector<Node*, ArenaAllocator<Node*>> keyArgs;
    Node* fn;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    GenericForNode(std::vector<Node*, ArenaAllocator<Node*>> ka, Node* f, std::vector<Node*, ArenaAllocator<Node*>> b) 
    : keyArgs(std::move(ka)), fn(std::move(f)), body(std::move(b)) {}

    std::string_view getName() const override {
        return "GenericForNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct RepeatUntilNode : Node {
    Node* condition;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    RepeatUntilNode(Node* c, std::vector<Node*, ArenaAllocator<Node*>> b) 
        : condition(std::move(c)), body(std::move(b)) { } 

    std::string_view getName() const override {
        return "RepeatUntilNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct FunctionNode : Node {
    Token value;
    bool isLocal = false;

    std::vector<Node*, ArenaAllocator<Node*>> args;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    FunctionNode(Token v, bool isL, std::vector<Node*, ArenaAllocator<Node*>> a, std::vector<Node*, ArenaAllocator<Node*>> b)
    : value(std::move(v)), isLocal(isL), args(std::move(a)), body(std::move(b)) {}
    
    std::string_view getName() const override {
        return "FunctionNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct AnonFunction : Node {
    std::vector<Node*, ArenaAllocator<Node*>> args;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    AnonFunction(std::vector<Node*, ArenaAllocator<Node*>> a, std::vector<Node*, ArenaAllocator<Node*>> b) 
    : args(std::move(a)), body(std::move(b)) { }

    std::string_view getName() const override {
        return "AnonFunction";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct MethodNode : Node {
    Token value;
    Token className;
    bool isLocal = false;
    
    std::vector<Node*, ArenaAllocator<Node*>> args;
    std::vector<Node*, ArenaAllocator<Node*>> body;

    MethodNode(Token v, Token cn, bool isL, std::vector<Node*, ArenaAllocator<Node*>> a, std::vector<Node*, ArenaAllocator<Node*>> b)
    : value(std::move(v)), className(std::move(cn)), isLocal(isL), args(std::move(a)), body(std::move(b)) {}

    std::string_view getName() const override {
        return "MethodNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct FunctionCallNode : Node {
    Node* callee;
    std::vector<Node*, ArenaAllocator<Node*>> args;

    FunctionCallNode(Node* c, std::vector<Node*, ArenaAllocator<Node*>> a) 
        : callee(std::move(c)), args(std::move(a)) { }

    std::string_view getName() const override {
        return "FunctionCallNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct MethodCallNode : Node {
    Token method_name;
    Node* object_name;
    std::vector<Node*, ArenaAllocator<Node*>> args;

    MethodCallNode(Token m, Node* o, std::vector<Node*, ArenaAllocator<Node*>> a) 
    : method_name(std::move(m)), object_name(std::move(o)), args(std::move(a)) {}

    std::string_view getName() const override {
        return "MethodCallNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

struct ReturnNode : Node {
    std::vector<Node*, ArenaAllocator<Node*>> args;

    ReturnNode(std::vector<Node*, ArenaAllocator<Node*>> a) : args(std::move(a)) { }

    std::string_view getName() const override {
        return "ReturnNode";
    }

    void accept(Visitor& v) override {
        v.visit(this);
    }
};

class Parser {
private:
    std::vector<Token> listOfTokens;
    unsigned long long index = 0;

    static constexpr Type endblockArray[5] = {Type::KW_END, Type::KW_ELSE,
                          Type::KW_ELSEIF, Type::KW_UNTIL,
                          Type::END_OF_FILE};

    static constexpr std::array<std::string_view, static_cast<size_t>(Type::ERROR) + 1> TokenToStrArray = {
        "'local'", "'if'", "'then'", "'else'", "'elseif'", "'end'",
        "'function'", "'return'", "'while'", "'const'", "'close'",
        "'for'", "'do'", "'repeat'", "'until'", "'nil'",
        "'true'", "'false'", "'and'", "'or'", "'not'", "'in'",
        "'integer literal'", "'float literal'", "'string literal'",
        "'hex literal'", "'char literal'", "'long string literal'",
        "'identificator'", "'function invocation'",
        "'+'", "'-'", "'*'", "'|'", "'/'", "'//'", "'%'",
        "'^'", "'.'", "'..'", "'#'", "'='", "'=='",
        "'~='", "'<'", "'>'", "'<='",
        "'>='", "'('", "')'", "'{'",
        "'}'", "'['", "']'", "','",
        "':'", "'::'", "';'", "'&'",
        "'!'", "'...'", "'~'", "'<<'", "'>>'",
        "'end of file'",
        "'ERROR'",
    };

    static constexpr std::array<Type, 5> UnaryOpSet = {
        Type::MINUS, Type::PLUS, Type::HASH, Type::KW_NOT, Type::TILDE
    };

    static inline const std::array<Type, 5> BitwiseOpSet = {
        Type::L_SHIFT, Type::R_SHIFT, Type::TILDE, Type::VERTICAL_BAR, Type::AMPERSAND
    };

    const Token& peek();
    const Token& peekNext();
    Token advance();
    bool check(Type type);
    bool checkNext(Type type); 
    void expect(Type type);
    [[noreturn]] void throwError(Type expected);
     
    bool endblock();
    int  get_lbp(); 
    Node* nud(); 
    Node* parse_expr(int min_lbp);

    Node* parse_if();
    Node* parse_elseif();
    Node* parse_do();
    Node* parse_while();
    Node* parse_for();
    Node* parse_repeat();
    Node* parse_return();
    Node* parse_local();
    Node* parse_function(bool isLocal);
    Node* parse_ident();
    Node* parse_method(Token className, bool isLocal); 

public:
    std::vector<Node*, ArenaAllocator<Node*>> parse_block(); 
    Node* parse_stat();

    Parser(std::vector<Token> VectorOfTokens) : listOfTokens(std::move(VectorOfTokens)) {}
};

#endif
