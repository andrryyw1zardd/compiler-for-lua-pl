#include "sema/sema.hpp"
#include "parser/parser.hpp"
#include <format>
#include <cassert>
#include <utility>
#include <iostream>

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

void Scope::add_into_symbols(const std::string& name, const Symbol& symbol) {
    variables_[name] = symbol;
}

bool Scope::has_locally(const std::string& name) const {
    return variables_.find(name) != variables_.end();
}

Scope* Scope::get_parent() {
    return this->parent_;
}

void SemanticAnalyzer::print_diags() {
    for (const auto& diag: *diags_.get_diags()) {
        std::cout << diag << std::endl;
    }
}

int SemanticAnalyzer::diag_count() {
    return diags_.get_diags()->size();
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

void SemanticAnalyzer::makeFuncScope(FunctionNode* fNode) {
    std::string vName = std::get<std::string>(fNode->value.value);
    Symbol symb = {
        .kind_ = (fNode->isLocal) ? Symbol::Kind::LOCAL : Symbol::Kind::GLOBAL,
        .data_type_ = Symbol::DataType::UNKNOWN,
        .is_used_ = false, 
        .node_ = fNode 
    };

    scopes_.back()->add_into_symbols(vName, symb);
    func_scopes_.push_back(fNode);
}

void SemanticAnalyzer::removeFuncScope() {
    assert(!func_scopes_.empty());
    func_scopes_.pop_back();
}

FunctionNode* SemanticAnalyzer::currentFuncScope() {
    assert(!func_scopes_.empty());
    return func_scopes_.back();
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

std::vector<std::string>* DiagnosticEngine::get_diags() {
    return &diag_vect_;
}

void SemanticAnalyzer::initGLobals() {
    std::array<std::string, 10> globalFuncNames {
        "print", "type", "tostring", "tonumber",
        "error", "assert", "select", "next",
        "pairs", "ipairs" 
    };

    std::array<std::vector<Symbol::DataType>, 10> globalFuncRetTypes {
        std::vector<Symbol::DataType>{Symbol::DataType::NIL}, // print
        std::vector<Symbol::DataType>{Symbol::DataType::STRING}, // type
        std::vector<Symbol::DataType>{Symbol::DataType::STRING}, // tostring
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // tonumber
        std::vector<Symbol::DataType>{Symbol::DataType::NIL}, // error
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // assert (it can return its args or error)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // select (it returns its own args, 0 or more)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN, Symbol::DataType::UNKNOWN}, // next (key, value)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // pairs (can return iter, table or nil, so just UNKNOWN)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // ipairs (same as pairs just 0 instead of nil)
    };

    for (size_t i = 0; i < globalFuncRetTypes.size(); ++i) {
        Symbol symb = {
            .kind_ = Symbol::Kind::GLOBAL,
            .data_type_ = Symbol::DataType::NIL,
            .return_types_ = globalFuncRetTypes[i],
            .is_used_ = false,
            .node_ = nullptr };

        scopes_.front()->add_into_symbols(globalFuncNames[i], symb);
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
        Symbol new_symb = {
            .kind_ = Symbol::Kind::GLOBAL,
            .data_type_ = Symbol::DataType::NIL,
            .is_used_ = true, 
            .node_ = vNode
        };
        std::string new_var_name = std::get<std::string>(vNode->value.value);
        scopes_.front()->add_into_symbols(new_var_name, new_symb);
    }
}

// add const attr handling
void SemanticAnalyzer::visit(MultipleVariableNode* mvNode) {
    std::vector<std::string> lNameVect;
    std::vector<Symbol::DataType> lDtVect;

    // this loop is for the left side to be correctly handled
    for (size_t i = 0; i < mvNode->left_side.size(); ++i) {
        auto left = mvNode->left_side[i];
        VariableNode* converted = dynamic_cast<VariableNode*>(left);

        if (!converted) {
            diags_.collect_diags(
                "expected variable name but got", std::string(left->getName()),
                DiagnosticEngine::DiagType::ERROR, left);
            continue;
        }

        std::string lName = std::get<std::string>(converted->value.value);
        lNameVect.push_back(lName);

        Symbol vSymb = {
            .kind_ = Symbol::Kind::LOCAL,
            .data_type_ = Symbol::DataType::UNKNOWN,
            .is_used_ = false, 
            .node_ = left 
        };

        if (mvNode->const_vect[i]) vSymb.have_const_attr = true;
        else vSymb.have_const_attr = false;

        scopes_.back()->add_into_symbols(lName, vSymb);
    }

    // and this loop if for the right side 
    for (const auto& right: mvNode->right_side) {
        right->accept(*this);
        FunctionCallNode* converted = dynamic_cast<FunctionCallNode*>(right);

        if (!converted || !converted->ret_data_types || converted->ret_data_types->empty()) {
            if (right->node_data_type) lDtVect.push_back(*right->node_data_type);
            else lDtVect.push_back(Symbol::DataType::UNKNOWN);
        }
        else {
            const auto& rets = converted->ret_data_types.value();

            if (right == mvNode->right_side.back()) 
                for (const auto& ret: rets) lDtVect.push_back(ret);
            else lDtVect.push_back(rets[0]);
        }
    }

    // finally this loop connects left and right sides
    for (size_t i = 0; i < lNameVect.size(); ++i) {
        auto lSymb = scopes_.back()->lookup(lNameVect[i], diags_);

        if (!lSymb) {
            assert(lSymb && "symbol was just added but lookup returned nullptr");
        }

        if (lDtVect.size() > i) { lSymb->data_type_ = lDtVect[i]; }
        else lSymb->data_type_ = Symbol::DataType::NIL;
    }
}

void SemanticAnalyzer::visit(IdentNode* iNode) {
    std::vector<std::string> lNameVect;
    std::vector<Symbol::DataType> lDtVect;

    for (const auto& left: iNode->left_side) {
        VariableNode* converted = dynamic_cast<VariableNode*>(left);

        if (!converted) {
            diags_.collect_diags(
                "expected variable name but got", std::string(left->getName()),
                DiagnosticEngine::DiagType::ERROR, left);
            continue;
        }

        std::string lName = std::get<std::string>(converted->value.value);
        lNameVect.push_back(lName);

        Symbol* symb = scopes_.back()->lookup(lName, diags_);

        if (!symb) {
            diags_.collect_diags(
                "undefined identificator", lName,
                DiagnosticEngine::DiagType::ERROR, left);
        }
        if (symb->have_const_attr.value_or(false)) {
            diags_.collect_diags(
                "variable with const attribute cant be modified. variable", lName,
                DiagnosticEngine::DiagType::ERROR, left);
        }
    }

    for (const auto& right: iNode->right_side) {
        right->accept(*this);
        FunctionCallNode* converted = dynamic_cast<FunctionCallNode*>(right);

        if (!converted || !converted->ret_data_types || converted->ret_data_types->empty()) {
            if (right->node_data_type) lDtVect.push_back(*right->node_data_type);
            else lDtVect.push_back(Symbol::DataType::UNKNOWN);
        }
        else {
            const auto& rets = converted->ret_data_types.value();

            if (right == iNode->right_side.back()) 
                for (const auto& ret: rets) lDtVect.push_back(ret);
            else lDtVect.push_back(rets[0]);
        }
    }
    
    for (size_t i = 0; i < lNameVect.size(); ++i) {
        auto lSymb = scopes_.back()->lookup(lNameVect[i], diags_);

        if (!lSymb) {
            assert(lSymb && "symbol was just added but lookup returned nullptr");
        }

        if (lDtVect.size() > i) { lSymb->data_type_ = lDtVect[i]; }
        else lSymb->data_type_ = Symbol::DataType::NIL;
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

        case Type::KW_TRUE:
        case Type::KW_FALSE:
            bdNode->node_data_type = Symbol::DataType::BOOL;
            break;
            
        default:
            bdNode->node_data_type = Symbol::DataType::UNKNOWN;
            break;
    }
}

void SemanticAnalyzer::visit(UnaryOpNode* uoNode) {
    uoNode->value->accept(*this);

    switch (uoNode->op.type) {
        case Type::MINUS:
        case Type::PLUS:
            if (uoNode->value->node_data_type == Symbol::DataType::FLOAT 
                ||uoNode->value->node_data_type == Symbol::DataType::INT
            ) {
                uoNode->node_data_type = uoNode->value->node_data_type;
            }
            else if (uoNode->value->node_data_type == Symbol::DataType::STRING) {
                uoNode->node_data_type = Symbol::DataType::FLOAT;
            }
            else {
                diags_.collect_diags(
                    "invalid data type of", std::string(uoNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, uoNode);
            }
            break;

        case Type::TILDE:
            if (uoNode->value->node_data_type == Symbol::DataType::INT 
                ||uoNode->value->node_data_type == Symbol::DataType::FLOAT
                ||uoNode->value->node_data_type == Symbol::DataType::STRING
            ) {
                uoNode->node_data_type = Symbol::DataType::INT;
            }
            else {
                diags_.collect_diags(
                    "invalid data type of", std::string(uoNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, uoNode);
            }
            break;
        case Type::KW_NOT:
            uoNode->node_data_type = Symbol::DataType::BOOL;
            break;

        // hash returns size of string/table
        case Type::HASH:
            if (uoNode->value->node_data_type == Symbol::DataType::TABLE 
                ||uoNode->value->node_data_type == Symbol::DataType::STRING
            ) {
                uoNode->node_data_type = Symbol::DataType::INT;
            }
            else {
                diags_.collect_diags(
                    "invalid data type of", std::string(uoNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, uoNode);
            }
            break;
            
            
        default:
            diags_.collect_diags(
                "invalid data type of", std::string(uoNode->getName()),
                DiagnosticEngine::DiagType::ERROR, uoNode);
            break;
    }
}

void SemanticAnalyzer::visit(MemberAccessNode* maNode) {
    maNode->value->accept(*this);

    std::string maName = std::get<std::string>(maNode->qualifier.value);
    Symbol* symb = scopes_.back()->lookup(maName, diags_);

    if (!symb) {
        diags_.collect_diags(
            "undeclerated qualifier", maName,
            DiagnosticEngine::DiagType::ERROR, maNode);
        return;
    }

    if (maNode->node_data_type == Symbol::DataType::UNKNOWN) {
        diags_.collect_diags(
            "unknown data type of", maName,
            DiagnosticEngine::DiagType::ERROR, maNode);
        return;
    }

    symb->is_used_ = true;
    maNode->node_data_type = maNode->value->node_data_type;
}

void SemanticAnalyzer::visit(ArrayNode* aNode) {
    for (const auto& el: aNode->elements) { el->accept(*this); }

    aNode->node_data_type = Symbol::DataType::TABLE;
}

void SemanticAnalyzer::visit(FunctionNode* fNode) {
    std::string funcName = std::get<std::string>(fNode->value.value);

    Symbol symb = {
        .kind_ = (fNode->isLocal) ? Symbol::Kind::LOCAL : Symbol::Kind::GLOBAL,
        .data_type_ = Symbol::DataType::UNKNOWN,
        .is_used_ = false, 
        .node_ = fNode 
    };
    scopes_.back()->add_into_symbols(funcName, symb);

    makeFuncScope(fNode);
    makeScope();
    for (auto& arg : fNode->args) {
        VariableNode* converted = dynamic_cast<VariableNode*>(arg);

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
            .data_type_ = Symbol::DataType::UNKNOWN,
            .is_used_ = false, 
            .node_ = arg 
        };

        scopes_.back()->add_into_symbols(vName, vSymb);
    }

    for (const auto& var : fNode->body) {
        var->accept(*this);
    }
    removeScope();
    removeFuncScope();
}

void SemanticAnalyzer::visit(ReturnNode* rNode) {
    std::vector<Symbol::DataType> temp_vect;

    for (const auto& iter : rNode->args) {
        iter->accept(*this);

        if (iter->node_data_type) {
            temp_vect.push_back(*iter->node_data_type);
        }
        else temp_vect.push_back(Symbol::DataType::UNKNOWN);
    }

    std::string fName = std::get<std::string>(currentFuncScope()->value.value);

    Symbol* symb = scopes_.back()->get_parent()->lookup(fName, diags_);
    if (!symb) {
        diags_.collect_diags(
            "function symbol not found in parent scope", 
            fName, DiagnosticEngine::DiagType::ERROR, rNode);
        return;
    }

    if (!symb->return_types_) {
        symb->return_types_ = temp_vect;
    } else {
        if (*symb->return_types_ != temp_vect) {
            diags_.collect_diags(
                "inconsistent/invalid return types in function", 
                fName, DiagnosticEngine::DiagType::ERROR, rNode);
        }
    }
}

void SemanticAnalyzer::visit(FunctionCallNode* fcNode) {
    for (const auto& arg: fcNode->args) {
        arg->accept(*this);
    }

    VariableNode* converted = dynamic_cast<VariableNode*>(fcNode->callee);
    if (!converted) {
        diags_.collect_diags(
            "invalid function call", std::string(fcNode->getName()),
            DiagnosticEngine::DiagType::ERROR, fcNode);
        return;
    }

    std::string fcName = std::get<std::string>(converted->value.value);

    auto symb = scopes_.back()->lookup(fcName, diags_);
    if (!symb) {
        diags_.collect_diags(
            "compiler: cant find function call name in symbol table", fcName,
            DiagnosticEngine::DiagType::ERROR, fcNode);
        return;
    }

    fcNode->ret_data_types = symb->return_types_;
}

void SemanticAnalyzer::visit(BinaryOpNode* boNode) {
    boNode->left->accept(*this);
    boNode->right->accept(*this);

    if (boNode->left->node_data_type == Symbol::DataType::UNKNOWN
     || boNode->right->node_data_type == Symbol::DataType::UNKNOWN)
    {
        diags_.collect_diags(
            "invalid data type of", 
            std::string(boNode->getName()),
            DiagnosticEngine::DiagType::ERROR, boNode);
        return;
    }

    switch (boNode->op) {
        case Type::EQUAL_EQUAL:
        case Type::NOT_EQUAL:
            boNode->node_data_type = Symbol::DataType::BOOL;
            break;

        case Type::GREATER:
        case Type::GREATER_EQUAL:
        case Type::LESS:
        case Type::LESS_EQUAL:
            if (boNode->left->node_data_type != Symbol::DataType::INT 
                && boNode->left->node_data_type != Symbol::DataType::FLOAT
                && boNode->left->node_data_type != Symbol::DataType::STRING) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }
            if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::FLOAT
                && boNode->right->node_data_type != Symbol::DataType::STRING) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }

            boNode->node_data_type = Symbol::DataType::BOOL;
            break;

        case Type::PLUS:
        case Type::MINUS:
        case Type::STAR:
        case Type::PERCENT:
        case Type::DOUBLE_SLASH:
            if (boNode->left->node_data_type != Symbol::DataType::INT 
                && boNode->left->node_data_type != Symbol::DataType::FLOAT) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }
            if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::FLOAT) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }
            
            if (boNode->left->node_data_type == Symbol::DataType::FLOAT 
                || boNode->right->node_data_type == Symbol::DataType::FLOAT) {
                boNode->node_data_type = Symbol::DataType::FLOAT;
            } 
            else boNode->node_data_type = Symbol::DataType::INT;
            break;

        // a .. b: here, a and b can be str, int or float 
        // the variables will be converted into strings
        // and they will be joined into one new string
        // so if a = "hello" and b = 12, then a..b = "hello12"
        case Type::CONCAT:
            if (boNode->left->node_data_type != Symbol::DataType::INT 
                && boNode->left->node_data_type != Symbol::DataType::FLOAT
                && boNode->left->node_data_type != Symbol::DataType::STRING) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }
            if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::FLOAT
                && boNode->right->node_data_type != Symbol::DataType::STRING) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }

            boNode->node_data_type = Symbol::DataType::STRING;
            break;

        // in lua, a^b and a/b always return float
        case Type::CARET:
        case Type::SLASH:
            if (boNode->left->node_data_type != Symbol::DataType::INT 
                && boNode->left->node_data_type != Symbol::DataType::FLOAT) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }
            if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::FLOAT) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
                break;
            }

            boNode->node_data_type = Symbol::DataType::FLOAT;
            break;

        default:
            diags_.collect_diags(
                "invalid data type of", 
                std::string(boNode->getName()),
                DiagnosticEngine::DiagType::ERROR, boNode);
            break;
    }
}

void SemanticAnalyzer::visit(AndTernaryNode* atNode) {
    atNode->left->accept(*this);
    atNode->right->accept(*this);

    if (atNode->left->node_data_type != Symbol::DataType::BOOL) {
        diags_.collect_diags(
            "invalid data type of", std::string(atNode->left->getName()),
            DiagnosticEngine::DiagType::ERROR, atNode->left);
        atNode->node_data_type = Symbol::DataType::NIL;
    }

    if (atNode->right->node_data_type != Symbol::DataType::BOOL) {
        diags_.collect_diags(
            "invalid data type of", std::string(atNode->right->getName()),
            DiagnosticEngine::DiagType::ERROR, atNode->right);
        atNode->node_data_type = Symbol::DataType::NIL;
    }

    if (!atNode->node_data_type) {
        atNode->node_data_type = Symbol::DataType::BOOL;
    }
}

void SemanticAnalyzer::visit(OrTernaryNode* atNode) {
    atNode->left->accept(*this);
    atNode->right->accept(*this);

    if (atNode->left->node_data_type != Symbol::DataType::BOOL ) {
        diags_.collect_diags(
            "invalid data type of", std::string(atNode->left->getName()),
            DiagnosticEngine::DiagType::ERROR, atNode->left);
        atNode->node_data_type = Symbol::DataType::NIL;
    }

    if (atNode->right->node_data_type != Symbol::DataType::BOOL ) {
        diags_.collect_diags(
            "invalid data type of", std::string(atNode->right->getName()),
            DiagnosticEngine::DiagType::ERROR, atNode->right);
        atNode->node_data_type = Symbol::DataType::NIL;
    }

    if (!atNode->node_data_type) {
        atNode->node_data_type = Symbol::DataType::BOOL;
    }
}

void SemanticAnalyzer::visit(BitwiseNode* bNode) {
    bNode->left->accept(*this);
    bNode->right->accept(*this);

    if (bNode->left->node_data_type == Symbol::DataType::UNKNOWN) {
        diags_.collect_diags( 
            "invalid data type of", std::string(bNode->getName()),
            DiagnosticEngine::DiagType::ERROR, bNode);
    }

    if (bNode->right->node_data_type == Symbol::DataType::UNKNOWN) {
        diags_.collect_diags( 
            "invalid data type of", std::string(bNode->getName()),
            DiagnosticEngine::DiagType::ERROR, bNode);
    }

    switch (bNode->op) {
        case Type::L_SHIFT:
        case Type::R_SHIFT:
        case Type::TILDE:
        case Type::AMPERSAND:
        case Type::VERTICAL_BAR:
            if ( bNode->left->node_data_type == Symbol::DataType::BOOL 
                || bNode->left->node_data_type == Symbol::DataType::NIL) 
            {
                diags_.collect_diags( 
                    "invalid data type of", std::string(bNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, bNode);
                break;
            }

            bNode->node_data_type = Symbol::DataType::INT;
            break;

        default: 
            diags_.collect_diags( 
                "invalid data type of", std::string(bNode->getName()),
                DiagnosticEngine::DiagType::ERROR, bNode);
            break;
    }
}
