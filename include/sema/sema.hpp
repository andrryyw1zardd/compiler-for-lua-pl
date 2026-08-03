#ifndef SEMA_HPP
#define SEMA_HPP

#include "parser/parser.hpp"

struct SymbolTable {};
struct ScopeStack {};
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
    SymbolTable globals;
    ScopeStack scopes;
    DiagnosticEngine& diags;

public:
    SemanticAnalyzer(DiagnosticEngine& d) : diags(d) { }
};

#endif
