#include "sema/sema.hpp"
#include "parser/parser.hpp"
#include <format>
#include <cassert>

void Scope::add_into_symbols(const std::string& name, const Symbol& symbol) {
    variables_[name] = symbol;
}

bool Scope::has_locally(const std::string& name) const {
    return variables_.find(name) != variables_.end();
}

Scope* Scope::get_parent() {
    return this->parent_;
}

Symbol* Scope::lookup(const std::string& name, DiagnosticEngine& de) {
    auto itValue = variables_.find(name);

    if (itValue != variables_.end()) {
        return &itValue->second;
    }

    if (parent_) {
        return parent_->lookup(name, de); 
    }

    return nullptr;
}

void DiagnosticEngine::collect_diags(
    const std::string& message,
    const std::string& name,
    const DiagType& diag,
    const Node* node) 
{
    std::string diag_str;

    if (diag == DiagType::ERROR) diag_str = "error";
    else diag_str = "warning";

    std::string formated_mes = std::format("[{}] {} '{}' at line {}, column {}.", 
                                           diag_str,
                                           message, name, 
                                           node->position.y,
                                           node->position.x);

    diag_vect_.push_back(formated_mes);
}

void SemanticAnalyzer::makeScope() {
    auto scope = std::make_unique<Scope>(
        scopes_.back().get());

    scopes_.push_back(std::move(scope));
}

void SemanticAnalyzer::removeScope() {
    assert(scopes_.back()->get_parent() != nullptr);
    scopes_.pop_back();
}

// by returning raw pointer of Scope, 
// I forse myself to work with current scope,
// without having opportunity to deleting it.
Scope* SemanticAnalyzer::currentScope() {
    return scopes_.back().get();
}

void SemanticAnalyzer::visit(DefineVariableNode* vNode) {
    Symbol symb = {
        .kind_ = Symbol::Kind::LOCAL,
        .is_used_ = false, 
        .node_ = vNode 
    };
    std::string val = std::get<std::string>(vNode->value.value);

    if (scopes_.back()->has_locally(val)) {
        diags_.collect_diags(
            "redefinition of variable", val,
             DiagnosticEngine::DiagType::ERROR, vNode
        );
    }
    else {
        vNode->right->accept(*this);
        scopes_.back()->add_into_symbols(std::move(val), std::move(symb));
    }
}

void SemanticAnalyzer::visit(VariableNode* vNode) {
    std::string val = std::get<std::string>(vNode->value.value);

    Symbol* symb = scopes_.back()->lookup(val, diags_);

    if (symb) {
        symb->is_used_ = true;
    }
    else {
        diags_.collect_diags(
            "undeclared identifier", val,
            DiagnosticEngine::DiagType::ERROR,
            vNode);
    }
}

void SemanticAnalyzer::visit(FunctionNode* fNode) {
    std::string funcName = std::get<std::string>(fNode->value.value);

    if (scopes_.back()->has_locally(funcName)) {
        diags_.collect_diags(
            "redefinition of", funcName,
            DiagnosticEngine::DiagType::ERROR,
            fNode);
    }

    Symbol symb = {
        .kind_ = (fNode->isLocal) ? Symbol::Kind::LOCAL : Symbol::Kind::GLOBAL,
        .is_used_ = false, 
        .node_ = fNode 
    };
    scopes_.back()->add_into_symbols(funcName, symb);

    makeScope();
    for (auto& var : fNode->args) {
        VariableNode* converted = dynamic_cast<VariableNode*>(var);

        // this if stat can work only if param of func was invalid
        // so for expample: function foo(1+2, val) ... end 
        // here, its not allowed to pass '1+2' as func param
        if (!converted) {
            diags_.collect_diags(
                "invalid function param in function", funcName,
                DiagnosticEngine::DiagType::ERROR, fNode);

            continue;
        }

        std::string vName = std::get<std::string>(converted->value.value);
        Symbol vSymb = {
            .kind_ = Symbol::Kind::PARAM,
            .is_used_ = false, 
            .node_ = fNode 
        };

        if (scopes_.back()->has_locally(vName)) {
            diags_.collect_diags(
                "invalid function param", funcName,
                DiagnosticEngine::DiagType::ERROR,
                converted);
        }
        else scopes_.back()->add_into_symbols(vName, vSymb);
    }

    for (const auto& var : fNode->body) {
        var->accept(*this);
    }
    removeScope();
}
