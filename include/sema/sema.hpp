#ifndef SEMA_HPP
#define SEMA_HPP

#include "parser/parser.hpp"
#include <vector>
#include <memory>

struct Symbol {
    enum class Kind { GLOBAL, PARAM, LOCAL, UPVALUE };
    Kind kind_;
    bool is_used_ = false;
    Node* node_ = nullptr;
};

class ScopeStack {
private:
    ScopeStack* parent_ = nullptr;
    std::unordered_map<std::string, Symbol> symbols_;

public:
    explicit ScopeStack(ScopeStack* p) : parent_(p) { } 
    ~ScopeStack() = default;

    void add_into_symbols(const std::string&, const Symbol&);
    bool has_locally(const std::string& name) const;
    Symbol* lookup(const std::string& name);
    ScopeStack* get_parent();
};


class DiagnosticEngine {
private:
    std::vector<std::string> diag_vect_;

public:
    enum class DiagType {ERROR, WARNING};
    void collect_diags(const std::string&,
                       const std::string&, 
                       const DiagType&,
                       const Node*);
};
 
class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(UnaryOpNode*) = 0;
    virtual void visit(BinaryOpNode*) = 0;
    virtual void visit(BitwiseNode*) = 0;
    virtual void visit(MemberAccessNode*) = 0;
    virtual void visit(BasicDataNode*) = 0;
    virtual void visit(VariableNode*) = 0;
    virtual void visit(VarWithAttributeNode*) = 0;
    virtual void visit(DefineVariableNode*) = 0;
    virtual void visit(MultipleVariableNode*) = 0;
    virtual void visit(AndTernaryNode*) = 0;
    virtual void visit(OrTernaryNode*) = 0;
    virtual void visit(ArrayNode*) = 0;
    virtual void visit(ExprWithIndexNode*) = 0;
    virtual void visit(IndexNode*) = 0;
    virtual void visit(XORNode*) = 0;
    virtual void visit(TableFieldNode*) = 0;
    virtual void visit(IfNode*) = 0;
    virtual void visit(ElseIfNode*) = 0;
    virtual void visit(DoNode*) = 0;
    virtual void visit(WhileNode*) = 0;
    virtual void visit(NumericForNode*) = 0;
    virtual void visit(GenericForNode*) = 0;
    virtual void visit(RepeatUntilNode*) = 0;
    virtual void visit(FunctionNode*) = 0;
    virtual void visit(AnonFunction*) = 0;
    virtual void visit(MethodNode*) = 0;
    virtual void visit(FunctionCallNode*) = 0;
    virtual void visit(MethodCallNode*) = 0;
    virtual void visit(ReturnNode*) = 0;
};

class SemanticAnalyzer : public Visitor {
private:
    std::vector<std::unique_ptr<ScopeStack>> scopes_;
    DiagnosticEngine& diags_;

public:
    SemanticAnalyzer(DiagnosticEngine& d) : diags_(d) { 
        scopes_.push_back(std::make_unique<ScopeStack>(nullptr));
    }
};

#endif
