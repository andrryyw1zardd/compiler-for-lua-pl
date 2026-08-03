#ifndef SEMA_HPP
#define SEMA_HPP

#include "parser/parser.hpp"
#include <vector>

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
    explicit ScopeStack(ScopeStack* p) : parent_(p) {} 
    ~ScopeStack() = default;

    void add_into_symbols(const std::string&, const Symbol&);
    bool has_locally(const std::string& name) const;
    Symbol* lookup(const std::string& name);
};

struct DiagnosticEngine {};
 
class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(UnaryOpNode*) = 0;
    virtual void visit(BinaryOpNode*) = 0;
    // ...
};

class SemanticAnalyzer : public Visitor {
private:
    std::vector<ScopeStack*> scopes;
    DiagnosticEngine& diags;

public:
    SemanticAnalyzer(DiagnosticEngine& d) : diags(d) { }
};

#endif
