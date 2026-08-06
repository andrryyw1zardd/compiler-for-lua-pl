#include "sema/sema.hpp"
#include "parser/parser.hpp"
#include <format>

void ScopeStack::add_into_symbols(const std::string& name, const Symbol& symbol) {
    symbols_[name] = symbol;
}

bool ScopeStack::has_locally(const std::string& name) const {
    return symbols_.find(name) != symbols_.end();
}

ScopeStack* ScopeStack::get_parent() {
    return this->parent_;
}

Symbol* ScopeStack::lookup(const std::string& name, DiagnosticEngine& de) {
    auto itValue = symbols_.find(name);

    if (itValue != symbols_.end()) {
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

    if (diag == DiagType::ERROR) diag_str = "Error";
    else diag_str = "Warning";

    std::string formated_mes = std::format("[{}] {} - {} at line {}, column {}.", 
                                           diag_str,
                                           message, name, 
                                           node->position.y,
                                           node->position.x);

    diag_vect_.push_back(formated_mes);
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
