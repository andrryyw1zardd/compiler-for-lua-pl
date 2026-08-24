#ifndef SEMA_HPP
#define SEMA_HPP

#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

struct Node; struct UnaryOpNode; struct BinaryOpNode; struct BitwiseNode;
struct MemberAccessNode; struct BasicDataNode; struct VariableNode;
struct VarWithAttributeNode; struct DefineVariableNode;
struct MultipleVariableNode; struct AndTernaryNode;
struct OrTernaryNode; struct ArrayNode; struct ExprWithIndexNode;
struct IndexNode; struct XORNode; struct TableFieldNode;
struct IfNode; struct ElseIfNode; struct DoNode; struct WhileNode;
struct NumericForNode; struct GenericForNode; struct RepeatUntilNode;
struct FunctionNode; struct AnonFunction; struct MethodNode;
struct FunctionCallNode; struct MethodCallNode; struct ReturnNode;

class DiagnosticEngine {
private:
    std::vector<std::string> diag_vect_;

public:
    enum class DiagType { ERROR, WARNING };
    void collect_diags(const std::string&,
                       const std::string&, 
                       const DiagType&,
                       const Node*);
};

struct Symbol {
    enum class Kind { GLOBAL, PARAM, LOCAL, UPVALUE };
    Kind kind_;

    enum class DataType { INT, FLOAT, TABLE, STRING, BOOL, NIL, UNKNOWN };
    DataType data_type_;
    std::optional<std::vector<DataType>> return_types_ = std::nullopt;

    bool is_used_ = false;
    Node* node_ = nullptr;
};

class Scope {
private:
    Scope* parent_ = nullptr;
    std::unordered_map<std::string, Symbol> variables_;

public:
    explicit Scope(Scope* p) : parent_(p) { } 
    ~Scope() = default;

    void add_into_symbols(const std::string&, const Symbol&);
    bool has_locally(const std::string& name) const;
    Symbol* lookup(const std::string& name, DiagnosticEngine& de);
    Scope* get_parent();
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
    std::vector<std::unique_ptr<Scope>> scopes_;
    std::vector<FunctionNode*> func_scopes_;
    DiagnosticEngine& diags_;

public:
    SemanticAnalyzer(DiagnosticEngine& d) : diags_(d) { 
        scopes_.push_back(std::make_unique<Scope>(nullptr));
        initGLobals();
    }

    void initGLobals();

    void makeScope();
    void removeScope();
    Scope* currentScope();

    void makeFuncScope(FunctionNode*);
    void removeFuncScope();
    FunctionNode* currentFuncScope();

    // variable things 
    void visit(DefineVariableNode*) override final;
    void visit(VariableNode*) override final;
    void visit(MultipleVariableNode*) override final;
    void visit(BasicDataNode*) override final;
    void visit(UnaryOpNode*) override final;
    void visit(MemberAccessNode*) override final;
    void visit(VarWithAttributeNode*) override final;

    // function things  
    void visit(FunctionNode*) override final;
    void visit(ReturnNode*) override final;
    void visit(FunctionCallNode*) override final;

    // operation things
    void visit(BinaryOpNode*) override final;
    void visit(AndTernaryNode*) override final;
    void visit(OrTernaryNode*) override final;
    void visit(BitwiseNode*) override final;
};

#endif
