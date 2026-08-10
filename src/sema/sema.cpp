#include "sema/sema.hpp"
#include "parser/parser.hpp"
#include <format>
#include <cassert>

void SemanticAnalyzer::makeScope() {
    auto scope = std::make_unique<Scope>(scopes_.back().get());
    scopes_.push_back(std::move(scope));
}

void SemanticAnalyzer::removeScope() {
    assert(scopes_.back()->get_parent() != nullptr);
    scopes_.pop_back();
}

Scope* SemanticAnalyzer::currentScope() {
    return scopes_.back().get();
}

void SemanticAnalyzer::makeFuncScope(FunctionNode* fNode) {
    func_scopes_.push_back(fNode);
}

void SemanticAnalyzer::removeFuncScope() {
    assert(func_scopes_.back()->get_parent() != nullptr);
    func_scopes_.pop_back();
}

FunctionNode* SemanticAnalyzer::currentFuncScope() {
    return func_scopes_.back();
}

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

void SemanticAnalyzer::visit(DefineVariableNode* vNode) {
    vNode->right->accept(*this);

    Symbol symb = {
        .kind_ = Symbol::Kind::LOCAL,
        .data_type_ = *vNode->right->node_data_type,
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
        scopes_.back()->add_into_symbols(std::move(val), std::move(symb));
    }
}

void SemanticAnalyzer::visit(VariableNode* vNode) {
    std::string val = std::get<std::string>(vNode->value.value);

    Symbol* symb = scopes_.back()->lookup(val, diags_);

    if (symb) {
        symb->is_used_ = true;
        vNode->node_data_type = symb->data_type_;
    }
    else {
        diags_.collect_diags(
            "undeclared identifier", val,
            DiagnosticEngine::DiagType::ERROR,
            vNode);
    }
}

void SemanticAnalyzer::visit(BasicDataNode* bdNode) {
    switch (bdNode->value.type) {
        case Type::LIT_INT:
        case Type::LIT_HEX: // need to parse hex to int in parser.cpp
            bdNode->node_data_type = Symbol::DataType::INT;
            break;

        case Type::LIT_STRING:
        case Type::LIT_LONG_STRING:
        case Type::LIT_CHAR:
            bdNode->node_data_type = Symbol::DataType::STRING;
            break;

        case Type::LIT_FLOAT:
            bdNode->node_data_type = Symbol::DataType::FLOAT;
            break;

        default:
            bdNode->node_data_type = Symbol::DataType::NULLTYPE;
            break;
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
        .data_type_ = Symbol::DataType::NULLTYPE,
        .is_used_ = false, 
        .node_ = fNode 
    };
    scopes_.back()->add_into_symbols(funcName, symb);

    makeFuncScope(fNode);
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
            .data_type_ = Symbol::DataType::NULLTYPE,
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
    removeFuncScope();
}
