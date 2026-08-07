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

    std::string formated_mes = std::format("[{}] {} - '{}' at line {}, column {}.", 
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
