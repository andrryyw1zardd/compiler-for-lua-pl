#include "sema/sema.hpp"
#include "parser/parser.hpp"
// #include "libs/std_lib.hpp"
#include <format>
#include <cassert>
#include <utility>
#include <iostream>

Scope* Scope::get_parent() {
    return this->parent_;
}

bool Scope::has_locally(const std::string& name) const {
    return variables_.find(name) != variables_.end();
}

void Scope::add_into_symbols(const std::string& name, const Symbol& symbol) {
    variables_[name] = symbol;
}

Symbol* Scope::lookup(const std::string& name) {
    auto itValue = variables_.find(name);

    if (itValue != variables_.end()) {
        return &itValue->second;
    }

    if (parent_) {
        return parent_->lookup(name); 
    }

    return nullptr;
}

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

void SemanticAnalyzer::print_diags() {
    for (const auto& diag: *diags_.get_diags()) {
        std::cout << diag << std::endl;
    }
}

int SemanticAnalyzer::diag_count() {
    return diags_.get_diags()->size();
}

Symbol* SemanticAnalyzer::getChildSymbol(MemberAccessNode* node) {
    Node* current = node;
    std::vector<std::string> q_str_vec = std::vector<std::string>{};

    while (auto converted = dynamic_cast<MemberAccessNode*>(current)) {
        q_str_vec.push_back(std::get<std::string>(converted->qualifier.value));
        current = converted->value;
    }

    auto base = dynamic_cast<VariableNode*>(current);
    if (!base) {
        diags_.collect_diags(
                "unexpected identificator with data type of",
                std::string(node->getName()),
                DiagnosticEngine::DiagType::ERROR, current);
        return nullptr;
    }

    std::string base_name = std::get<std::string>(base->value.value);
    Symbol* base_symb = scopes_.back()->lookup(base_name);
    if (!base_symb) {
        diags_.collect_diags(
                "use of undefined identificator",
                std::get<std::string>(base->value.value),
                DiagnosticEngine::DiagType::ERROR, base);
        return nullptr;
    }

    Symbol* iter_symb = base_symb;

    for (int i = q_str_vec.size()-1; i >= 0; i--) {
        Symbol* child_symb = nullptr;

        if (iter_symb->method_map_.has_value()) {
            for (size_t j = 0; j < iter_symb->method_map_.value().size(); j++) {
                if (q_str_vec[i] == iter_symb->method_map_.value()[j].name_) {
                    child_symb = &iter_symb->method_map_.value()[j];
                    break;
                }
            }
        }

        if (!child_symb) {
            if (iter_symb->variable_map_.has_value()) {
                for (size_t j = 0; j < iter_symb->variable_map_.value().size(); j++) {
                    if (q_str_vec[i] == iter_symb->variable_map_.value()[j].name_) {
                        child_symb = &iter_symb->variable_map_.value()[j];
                        break;
                    }
                }
            }
        }

        if (child_symb) iter_symb = child_symb;
        else {
            diags_.collect_diags(
                    "use of undefined qualifier of", base_name,
                    DiagnosticEngine::DiagType::ERROR, base);
            return nullptr;
        }
    }

    return iter_symb;
}

Symbol* SemanticAnalyzer::getSymbolFromNode(Node* node) {
    VariableNode* node_variable = dynamic_cast<VariableNode*>(node);

    if (node_variable) {
        Symbol* symb = scopes_.back()->lookup(std::get<std::string>(node_variable->value.value));

        if (!symb) {
            diags_.collect_diags(
                "undefined identificator with name",
                std::get<std::string>(node_variable->value.value),
                DiagnosticEngine::DiagType::ERROR, node);
            return nullptr;
        }

        return symb;
    }

    MemberAccessNode* node_member = dynamic_cast<MemberAccessNode*>(node);

    if (node_member) {
        Symbol* symb = getChildSymbol(node_member);

        if (!symb) { return nullptr; }
        return symb;
    }

    diags_.collect_diags(
            "undefined symb with data type of",
            std::string(node->getName()),
            DiagnosticEngine::DiagType::ERROR, node);
    return nullptr;
}

Symbol* SemanticAnalyzer::getChildSymbByName(Symbol& parent, std::string child_name) {
    if (child_name == "") return &parent;
    if (!parent.variable_map_.has_value()) return nullptr;

    for (auto& ch: parent.variable_map_.value()) {
        if (ch.name_ == child_name) {
            return &ch;
        }
    }

    return nullptr;
}

void SemanticAnalyzer::makeFuncScope(Node* node) {
    FunctionNode* fNode = dynamic_cast<FunctionNode*>(node);
    AnonFunctionNode* afNode = dynamic_cast<AnonFunctionNode*>(node);
    MethodNode* mNode = dynamic_cast<MethodNode*>(node);

    if (fNode) {
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
    else if (mNode) {
        func_scopes_.push_back(mNode);
    }
    else if (afNode) {
        func_scopes_.push_back(afNode);
    }
    else {
        diags_.collect_diags(
            "compiler error", "could not make a function scope", 
            DiagnosticEngine::DiagType::ERROR, node
        );
    }
}

void SemanticAnalyzer::removeFuncScope() {
    assert(!func_scopes_.empty());
    func_scopes_.pop_back();
}

std::variant<FunctionNode*, AnonFunctionNode*, MethodNode*> SemanticAnalyzer::currentFuncScope() {
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

void SemanticAnalyzer::initGlobalFunctions() {
    std::array<std::string, 13> globalFuncNames {
        "print", "type", "tostring", "tonumber",
        "error", "assert", "select", "next",
        "pairs", "ipairs", "pcall", "xpcall",
        "setmetatable"
    };

    std::vector<std::vector<Symbol::DataType>> globalFuncRetTypes {
        std::vector<Symbol::DataType>{Symbol::DataType::NIL}, // print
        std::vector<Symbol::DataType>{Symbol::DataType::STRING}, // type
        std::vector<Symbol::DataType>{Symbol::DataType::STRING}, // tostring
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // tonumber
        std::vector<Symbol::DataType>{Symbol::DataType::NIL}, // error
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // assert (it can return its args or error)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // select (it returns its own args, 0 or more)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN, Symbol::DataType::UNKNOWN}, // next (key, value)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN}, // pairs (can return iter, table or nil, so just UNKNOWN)
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN,
            Symbol::DataType::UNKNOWN, Symbol::DataType::UNKNOWN}, // ipairs (same as pairs just 0 instead of nil)
        std::vector<Symbol::DataType>{Symbol::DataType::BOOL, Symbol::DataType::UNKNOWN}, // pcall
        std::vector<Symbol::DataType>{Symbol::DataType::BOOL, Symbol::DataType::UNKNOWN}, // xpcall
        std::vector<Symbol::DataType>{Symbol::DataType::UNKNOWN} // setmetatable
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

void SemanticAnalyzer::initGlobalMethods() {
    using DT = Symbol::DataType;
     
    std::vector<std::string> globalIdentNames = {
        "math", "string", "table", "os", "io", "coroutine", "utf8", "debug",
        "package", "file" };
     
    std::vector<std::vector<std::string>> globalMethodNames = {
        { "abs", "ceil", "floor", "sqrt", "sin", "cos", "tan", "asin", "acos",
          "atan", "exp", "log", "fmod", "modf", "max", "min", "random",
          "randomseed", "tointeger", "type", "ult" },
        { "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len",
          "lower", "match", "pack", "packsize", "rep", "reverse", "sub",
          "upper", "unpack" },
        { "concat", "insert", "move", "pack", "remove", "sort", "unpack" },
        { "clock", "date", "difftime", "execute", "exit", "getenv", "remove",
          "rename", "setlocale", "time", "tmpname" },
        { "close", "flush", "input", "lines", "open", "output", "popen",
          "read", "tmpfile", "write" },
        { "create", "isyieldable", "resume", "running", "status", "wrap",
          "yield", "close" },
        { "char", "codepoint", "codes", "len", "offset" },
        { "debug", "gethook", "getinfo", "getlocal", "getmetatable",
          "getregistry", "getupvalue", "getuservalue", "sethook", "setlocal",
          "setmetatable", "setupvalue", "setuservalue", "traceback",
          "upvalueid", "upvaluejoin" },
        { "loadlib", "searchpath" },
        { "close", "flush", "lines", "read", "seek", "setvbuf", "write" }
    };
     
    std::vector<std::vector<DT>> globalMethodRetTypes = {
        { DT::FLOAT, DT::INT, DT::INT, DT::FLOAT, DT::FLOAT, DT::FLOAT,
          DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::FLOAT,
          DT::FLOAT, DT::UNKNOWN, DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::NIL,
          DT::UNKNOWN, DT::STRING, DT::BOOL
        },
        { DT::UNKNOWN, DT::STRING, DT::STRING, DT::UNKNOWN, DT::STRING,
          DT::FUNCTION, DT::UNKNOWN, DT::INT, DT::STRING, DT::UNKNOWN,
          DT::STRING, DT::INT, DT::STRING, DT::STRING, DT::STRING,
          DT::STRING, DT::UNKNOWN 
        },
        { DT::STRING, DT::NIL, DT::TABLE, DT::TABLE, DT::UNKNOWN, DT::NIL,
          DT::UNKNOWN 
        },
        { DT::FLOAT, DT::UNKNOWN, DT::FLOAT, DT::UNKNOWN, DT::NIL,
          DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::INT,
          DT::STRING 
        },
        { DT::BOOL, DT::BOOL, DT::UNKNOWN, DT::FUNCTION, DT::UNKNOWN,
          DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN 
        },
        { DT::UNKNOWN, DT::BOOL, DT::UNKNOWN, DT::UNKNOWN, DT::STRING,
          DT::FUNCTION, DT::UNKNOWN, DT::BOOL 
        },
        { DT::STRING, DT::UNKNOWN, DT::FUNCTION, DT::UNKNOWN, DT::INT },
        { DT::NIL, DT::UNKNOWN, DT::TABLE, DT::UNKNOWN, DT::TABLE, DT::TABLE,
          DT::UNKNOWN, DT::UNKNOWN, DT::NIL, DT::STRING, DT::UNKNOWN,
          DT::STRING, DT::UNKNOWN, DT::STRING, DT::UNKNOWN, DT::NIL 
        },
        { DT::UNKNOWN, DT::UNKNOWN },
        { DT::BOOL, DT::UNKNOWN, DT::FUNCTION, DT::UNKNOWN, DT::UNKNOWN,
          DT::BOOL, DT::UNKNOWN }
    };


    for (size_t i = 0; i < globalMethodRetTypes.size(); ++i) {
        Symbol parent_symb = {
            .kind_ = Symbol::Kind::GLOBAL,
            .data_type_ = Symbol::DataType::NIL,
            .name_ = globalIdentNames[i],
            .is_used_ = false,
            .node_ = nullptr 
        };

        parent_symb.method_map_.emplace();
        parent_symb.method_map_->reserve(globalMethodNames[i].size());

        for (size_t j = 0; j < globalMethodNames[i].size(); ++j) {
            Symbol method_symb = {
                .kind_ = Symbol::Kind::LOCAL,
                .data_type_ = globalMethodRetTypes[i][j],
                .name_ = globalMethodNames[i][j],
            };

            method_symb.return_types_.emplace();
            method_symb.return_types_->push_back(globalMethodRetTypes[i][j]);

            parent_symb.method_map_->push_back(method_symb);
        }

        scopes_.front()->add_into_symbols(globalIdentNames[i], parent_symb);
    }
}

void SemanticAnalyzer::visit(VariableNode* vNode) {
    std::string val = std::get<std::string>(vNode->value.value);

    Symbol* symb = scopes_.back()->lookup(val);

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

        if (i < mvNode->const_vect.size() && mvNode->const_vect[i]) vSymb.have_const_attr_ = true;
        else vSymb.have_const_attr_ = false;

        scopes_.back()->add_into_symbols(lName, vSymb);
    }

    // and this loop if for the right side 
    for (size_t i = 0; i < mvNode->right_side.size(); ++i) {
        auto right = mvNode->right_side[i];
        right->accept(*this);

        FunctionCallNode* converted = dynamic_cast<FunctionCallNode*>(right);
        AnonFunctionNode* converted_anon_func = dynamic_cast<AnonFunctionNode*>(right);
        ArrayNode* converted_arr = dynamic_cast<ArrayNode*>(right);

        if (converted && converted->ret_data_types && !converted->ret_data_types->empty()) {
            const auto& rets = converted->ret_data_types.value();

            if (right == mvNode->right_side.back()) 
                for (const auto& ret: rets) lDtVect.push_back(ret);
            else lDtVect.push_back(rets[0]);

            continue;
        }

        if (converted_arr) {
            if (i < lNameVect.size()) {
                Symbol* symb = scopes_.back()->lookup(lNameVect[i]);

                if (!symb) {
                    diags_.collect_diags(
                        "undeclerated array definition, variable", lNameVect[i], 
                        DiagnosticEngine::DiagType::ERROR, right);
                    return;
                }

                symb->variable_map_ = converted_arr->symbols_vector;
            }

            lDtVect.push_back(Symbol::DataType::TABLE);
            continue;
        }

        if (converted_anon_func) {
            if (i < lNameVect.size()) {
                Symbol* symb = scopes_.back()->lookup(lNameVect[i]);

                if (!symb) {
                    diags_.collect_diags(
                        "undeclerated function definition, variable", lNameVect[i], 
                        DiagnosticEngine::DiagType::ERROR, right);
                    return;
                }

                symb->return_types_ = converted_anon_func->return_types;
            }

            lDtVect.push_back(Symbol::DataType::FUNCTION);
            continue;
        }

        if (right->node_data_type) lDtVect.push_back(*right->node_data_type);
        else lDtVect.push_back(Symbol::DataType::UNKNOWN);
    }

    // finally, this loop connects left and right sides
    for (size_t i = 0; i < lNameVect.size(); ++i) {
        auto lSymb = scopes_.back()->lookup(lNameVect[i]);

        if (!lSymb) {
            assert(lSymb && "symbol was just added but lookup returned nullptr");
        }

        if (lDtVect.size() > i) { lSymb->data_type_ = lDtVect[i]; }
        else lSymb->data_type_ = Symbol::DataType::NIL;
    }
}

void SemanticAnalyzer::visit(IdentNode* iNode) {
    std::vector<Symbol*> lSymbVect;
    std::vector<std::string> lFieldNameVect;
    std::vector<Symbol::DataType> lDtVect;

    for (const auto& left: iNode->left_side) {
        VariableNode* converted_variable = dynamic_cast<VariableNode*>(left);
        ExprWithIndexNode* converted_with_index = dynamic_cast<ExprWithIndexNode*>(left);
        MemberAccessNode* converted_member = dynamic_cast<MemberAccessNode*>(left);
        std::string lName = "";

        if (converted_variable) {
            Node* current = converted_variable;

            while (auto converted = dynamic_cast<MemberAccessNode*>(current)) {
                // check the current
                current = converted->value;
            }

            VariableNode* base = dynamic_cast<VariableNode*>(current);
            if (!base) {
                diags_.collect_diags(
                    "access on non-indentifier expression", std::string(current->getName()),
                    DiagnosticEngine::DiagType::ERROR, iNode);
                return;
            }

            lName = std::get<std::string>(base->value.value);

            if (!scopes_.back()->lookup(lName)) {
                // diags
                return;
            }

            Symbol* symb = scopes_.back()->lookup(lName);

            symb->name_ = lName;
            lSymbVect.push_back(symb);
            lFieldNameVect.push_back("");
        }

        else if (converted_with_index) {
            Node* current = converted_with_index;

            while (auto converted = dynamic_cast<MemberAccessNode*>(current)) {
                // need to check the current
                current = converted->value;
            }

            ExprWithIndexNode* base = dynamic_cast<ExprWithIndexNode*>(current);
            if (!base) {
                diags_.collect_diags(
                    "access on non-indentifier expression", std::string(current->getName()),
                    DiagnosticEngine::DiagType::ERROR, iNode);
                return;
            }

            VariableNode* left_base = dynamic_cast<VariableNode*>(base->left);
            if (!left_base) {
                diags_.collect_diags(
                    "access on non-indentifier expression", std::string(current->getName()),
                    DiagnosticEngine::DiagType::ERROR, iNode);
                return;
            }

            base->index_expr->accept(*this);

            lName = std::get<std::string>(left_base->value.value);

            if (!scopes_.back()->lookup(lName)) {
                // diags
                return;
            }

            Symbol* symb = scopes_.back()->lookup(lName);
            lSymbVect.push_back(symb);
            lFieldNameVect.push_back("");
        }
        else if (converted_member) {
            Node* current = converted_member;

            while (auto converted = dynamic_cast<MemberAccessNode*>(current)) {
                current = converted->value;
            }

            VariableNode* base = dynamic_cast<VariableNode*>(current);
            if (!base) {
                diags_.collect_diags(
                    "access on non-indentifier expression", std::string(current->getName()),
                    DiagnosticEngine::DiagType::ERROR, iNode);
                return;
            }

            std::string base_name = std::get<std::string>(base->value.value);
            lName = base_name;

            Symbol* base_symb = scopes_.back()->lookup(base_name);

            if (!base_symb) {
                // diags
                return;
            }

            if (base_symb->variable_map_.has_value() && base_symb->variable_map_->size() > 0) {
                bool found = false;
                std::string qs_name = std::get<std::string>(converted_member->qualifier.value);

                for (auto& qs: *base_symb->variable_map_) {
                    if (qs.name_ == qs_name) {
                        lSymbVect.push_back(base_symb);
                        lFieldNameVect.push_back(qs_name);
                        found = true;
                    }
                }

                if (!found) {
                    // dt will be regiven in last for loop
                    Symbol new_qs = { 
                        .kind_ = Symbol::Kind::LOCAL,
                        .data_type_ = Symbol::DataType::NIL,
                        .name_ = qs_name,
                        .is_used_ = true
                    };

                    base_symb->variable_map_->push_back(new_qs);
                    lSymbVect.push_back(base_symb);
                    lFieldNameVect.push_back(qs_name);
                }
            }
            else {
                base_symb->variable_map_.emplace();

                std::string qs_name = std::get<std::string>(converted_member->qualifier.value);
                Symbol new_qs = { 
                    .kind_ = Symbol::Kind::LOCAL,
                    .data_type_ = Symbol::DataType::NIL,
                    .name_ = qs_name,
                    .is_used_ = true
                };

                base_symb->variable_map_->push_back(new_qs);

                lSymbVect.push_back(base_symb);
                lFieldNameVect.push_back(qs_name);
            }
        }
        else {
            diags_.collect_diags(
                "expected variable name but got", std::string(left->getName()),
                DiagnosticEngine::DiagType::ERROR, left);
            continue;
        }

        Symbol* symb = scopes_.back()->lookup(lName);

        if (!symb) {
            diags_.collect_diags(
                "undefined identificator", lName,
                DiagnosticEngine::DiagType::ERROR, left);
            continue;
        }
        if (symb->have_const_attr_.value_or(false)) {
            diags_.collect_diags(
                "variable with const attribute cant be modified. variable", lName,
                DiagnosticEngine::DiagType::ERROR, left);
            continue;
        }
    }

    for (size_t i = 0; i < iNode->right_side.size(); ++i) {
        auto right = iNode->right_side[i];
        right->accept(*this);

        FunctionCallNode* converted = dynamic_cast<FunctionCallNode*>(right);
        ArrayNode* converted_arr = dynamic_cast<ArrayNode*>(right);
        AnonFunctionNode* converted_anon_func = dynamic_cast<AnonFunctionNode*>(right);

        if (converted && converted->ret_data_types && !converted->ret_data_types->empty()) {
            const auto& rets = converted->ret_data_types.value();

            if (right == iNode->right_side.back()) 
                for (const auto& ret: rets) lDtVect.push_back(ret);
            else lDtVect.push_back(rets[0]);

            continue;
        }

        if (converted_arr) {
            if (i < lSymbVect.size()) {
                Symbol* symb = getChildSymbByName(*lSymbVect[i], lFieldNameVect[i]);

                if (!symb) {
                    diags_.collect_diags(
                        "undeclerated array definition, variable",
                        *symb->name_, 
                        DiagnosticEngine::DiagType::ERROR, right);
                    return;
                }

                symb->variable_map_ = converted_arr->symbols_vector;
            }

            lDtVect.push_back(Symbol::DataType::TABLE);
            continue;
        }

        if (converted_anon_func) {
            if (i < lSymbVect.size()) {
                Symbol* symb = getChildSymbByName(*lSymbVect[i], lFieldNameVect[i]);

                if (!symb) {
                    diags_.collect_diags(
                        "undeclerated function definition, variable", *symb->name_,
                        DiagnosticEngine::DiagType::ERROR, right);
                    return;
                }

                symb->return_types_ = converted_anon_func->return_types;
            }
        }

        if (right->node_data_type) lDtVect.push_back(*right->node_data_type);
        else lDtVect.push_back(Symbol::DataType::UNKNOWN);
    }
    
    for (size_t i = 0; i < lSymbVect.size(); ++i) {
        Symbol* lSymb = getChildSymbByName(*lSymbVect[i], lFieldNameVect[i]);

        if (!lSymb) {
            // diags
            continue;
        }
        else if (lSymb->data_type_ == Symbol::DataType::TABLE) { continue; }

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
            bdNode->node_data_type = Symbol::DataType::NIL;
            break;
    }
}

void SemanticAnalyzer::visit(UnaryOpNode* uoNode) {
    uoNode->value->accept(*this);

    switch (uoNode->op.type) {
        case Type::MINUS:
        case Type::PLUS:
            if (uoNode->value->node_data_type == Symbol::DataType::FLOAT 
                || uoNode->value->node_data_type == Symbol::DataType::INT
                || uoNode->value->node_data_type == Symbol::DataType::UNKNOWN
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
                || uoNode->value->node_data_type == Symbol::DataType::FLOAT
                || uoNode->value->node_data_type == Symbol::DataType::STRING
                || uoNode->value->node_data_type == Symbol::DataType::UNKNOWN
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
                || uoNode->value->node_data_type == Symbol::DataType::STRING
                || uoNode->value->node_data_type == Symbol::DataType::UNKNOWN
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
    Symbol* symb = getSymbolFromNode(maNode->value);
    Symbol* qual_symb = nullptr;
    if (!symb) { return; }

    if (symb->data_type_ == Symbol::DataType::UNKNOWN) {
        maNode->node_data_type = Symbol::DataType::UNKNOWN;
        return;
    }

    if (symb->data_type_ == Symbol::DataType::TABLE) {
        std::string qual_name = std::get<std::string>(maNode->qualifier.value);

        if (!symb->variable_map_.has_value()) {
            symb->variable_map_.emplace();
        }

        bool contains_qual = false;

        for (auto& qual: *symb->variable_map_) {
            if (qual.name_ == qual_name) {
                contains_qual = true;
                qual_symb = &qual;
            }
        }

        if (!contains_qual) {
            symb->variable_map_->push_back(
                    Symbol{
                        .kind_ = Symbol::Kind::LOCAL,
                        .data_type_ = Symbol::DataType::NIL,
                        .name_  = qual_name,
                    });

            qual_symb = &symb->variable_map_->back();
        }

        maNode->node_data_type = qual_symb->data_type_;
        return;
    }

    Symbol* qual = nullptr;
    std::string qual_name = std::get<std::string>(maNode->qualifier.value);

    if (!symb->variable_map_.has_value()) {
        diags_.collect_diags(
                "use of undefined qualifier", 
                std::get<std::string>(maNode->qualifier.value),
                DiagnosticEngine::DiagType::ERROR, maNode);
        return;
    }

    for (auto& name: symb->variable_map_.value()) {
        if (qual_name == name.name_) {
            qual = &name;
            break;
        }
    }

    if (!qual) {
        diags_.collect_diags(
               "use of undefined qualifier", qual_name,
                DiagnosticEngine::DiagType::ERROR, maNode);

        maNode->node_data_type = Symbol::DataType::NIL;
        return;
    }

    maNode->node_data_type = qual->data_type_;
}

void SemanticAnalyzer::visit(EllipsisNode* eNode) {
    eNode->node_data_type = Symbol::DataType::ELLIPSIS;
}

void SemanticAnalyzer::visit(ArrayNode* aNode) {
    std::optional<std::vector<Symbol>> nasted_symbols = std::vector<Symbol>{};
    int index = 1;

    for (const auto& el: aNode->elements) {
        el->accept(*this);
        TableFieldNode* el_converted = dynamic_cast<TableFieldNode*>(el);

        if (el_converted) {
            IndexNode* el_key_converted_index = dynamic_cast<IndexNode*>(el_converted->key);
            VariableNode* el_key_converted_var = dynamic_cast<VariableNode*>(el_converted->key);

            if (el_key_converted_index) {
                BasicDataNode* index_expr_converted = dynamic_cast<BasicDataNode*>(el_key_converted_index->index_expr);

                if (index_expr_converted) {
                    if (index_expr_converted->value.type == Type::LIT_INT 
                        ||index_expr_converted->value.type == Type::LIT_HEX)
                    {
                        int name = std::get<int>(index_expr_converted->value.value);

                        if (el_converted->value->node_data_type.has_value()) {
                            nasted_symbols->push_back(Symbol {
                                .kind_ = Symbol::Kind::LOCAL,
                                .data_type_ = *el_converted->value->node_data_type,
                                .name_ = std::format("{}", name),
                                .node_ = index_expr_converted,
                            });
                        }
                        else {
                            nasted_symbols->push_back(Symbol {
                                .kind_ = Symbol::Kind::LOCAL,
                                .data_type_ = Symbol::DataType::UNKNOWN, 
                                .name_ = std::format("{}", name),
                                .node_ = index_expr_converted,
                            });
                        }
                    }

                    if (index_expr_converted->value.type == Type::LIT_STRING 
                        ||index_expr_converted->value.type == Type::LIT_LONG_STRING
                        ||index_expr_converted->value.type == Type::LIT_CHAR) 
                    {
                        std::string name = std::get<std::string>(index_expr_converted->value.value);

                        if (el_converted->value->node_data_type.has_value()) {
                            nasted_symbols->push_back(Symbol {
                                .kind_ = Symbol::Kind::LOCAL,
                                .data_type_ = *el_converted->value->node_data_type, 
                                .name_ = name,
                                .node_ = index_expr_converted,
                            });
                        }
                        else {
                            nasted_symbols->push_back(Symbol {
                                .kind_ = Symbol::Kind::LOCAL,
                                .data_type_ = Symbol::DataType::UNKNOWN, 
                                .name_ = name,
                                .node_ = index_expr_converted,
                            });
                        }
                    }
                }
            }

            else if (el_key_converted_var) {
                std::string key_name = std::get<std::string>(el_key_converted_var->value.value);

                if (el_converted->value->node_data_type.has_value()) {
                    nasted_symbols->push_back(Symbol {
                        .kind_ = Symbol::Kind::LOCAL,
                        .data_type_ = *el_converted->value->node_data_type, 
                        .name_ = key_name,
                        .node_ = el_key_converted_var,
                    });
                }
                else {
                    nasted_symbols->push_back(Symbol {
                        .kind_ = Symbol::Kind::LOCAL,
                        .data_type_ = Symbol::DataType::UNKNOWN, 
                        .name_ = key_name,
                        .node_ = el_key_converted_var,
                    });
                }

                if (auto* nasted_arr = dynamic_cast<ArrayNode*>(el_converted->value)) {
                    nasted_symbols->back().variable_map_ = nasted_arr->symbols_vector;
                }
            }

            else {
                diags_.collect_diags(
                    "unexpected node type", std::string(el_converted->getName()),
                    DiagnosticEngine::DiagType::ERROR, el_converted);
            }
        }

        // just an element
        else {
            if (el->node_data_type.has_value()) {
                nasted_symbols->push_back(Symbol {
                    .kind_ = Symbol::Kind::LOCAL,
                    .data_type_ = *el->node_data_type, 
                    .name_ = std::format(" ", index++),
                    .node_ = el,
                });
            }
            else {
                nasted_symbols->push_back(Symbol {
                    .kind_ = Symbol::Kind::LOCAL,
                    .data_type_ = Symbol::DataType::UNKNOWN, 
                    .name_ = std::format(" ", index++),
                    .node_ = el,
                });
            } 
        }
    }

    aNode->node_data_type = Symbol::DataType::TABLE;
    aNode->symbols_vector = nasted_symbols;
}

void SemanticAnalyzer::visit(TableFieldNode* tfNode) {
    tfNode->value->accept(*this);
    tfNode->node_data_type = tfNode->value->node_data_type;
}

void SemanticAnalyzer::visit(ExprWithIndexNode* eNode) {
    VariableNode* converted_left = dynamic_cast<VariableNode*>(eNode->left);

    if (!converted_left) {
        diags_.collect_diags(
            "expected data type 'table' but got", std::string(eNode->left->getName()),
            DiagnosticEngine::DiagType::ERROR, eNode);
        return;
    }

    std::string name_left = std::get<std::string>(converted_left->value.value);
    Symbol* symb = scopes_.back()->lookup(name_left);

    if (!symb) {
        diags_.collect_diags(
            "undefined indentificator", name_left,
            DiagnosticEngine::DiagType::ERROR, converted_left);
        return;
    }

    if (symb->data_type_ != Symbol::DataType::TABLE || !symb->variable_map_) {
        diags_.collect_diags(
            "expected data type 'table' but got", name_left,
            DiagnosticEngine::DiagType::ERROR, converted_left);
        return;
    }

    symb->is_used_ = true;
    eNode->index_expr->accept(*this);

    BasicDataNode* converted_ind_bd = dynamic_cast<BasicDataNode*>(eNode->index_expr);
    VariableNode* converted_ind_var = dynamic_cast<VariableNode*>(eNode->index_expr);
    BinaryOpNode* converted_ind_bo = dynamic_cast<BinaryOpNode*>(eNode->index_expr);

    if (converted_ind_bd) {
        converted_ind_bd->accept(*this);
        eNode->node_data_type = Symbol::DataType::UNKNOWN;
        return;
    }
    else if (converted_ind_bo) {
        converted_ind_bo->accept(*this);
        eNode->node_data_type = Symbol::DataType::UNKNOWN;
        return;
    }
    else {
        eNode->node_data_type = Symbol::DataType::UNKNOWN;
        return;
    }

    if (converted_ind_var) {
        if (converted_ind_var->value.type == Type::LIT_INT 
            || converted_ind_var->value.type == Type::LIT_HEX 
            || converted_ind_var->value.type == Type::LIT_STRING 
            || converted_ind_var->value.type == Type::LIT_CHAR 
            || converted_ind_var->value.type == Type::LIT_LONG_STRING) 
        {
            auto value = std::get<std::string>(converted_ind_var->value.value);
            Symbol::DataType* dt = nullptr;

            for (auto& el_symb: *symb->variable_map_) {
                if (el_symb.name_ == value) {
                    dt = &el_symb.data_type_;
                }
            }

            if (!dt) {
                eNode->node_data_type = Symbol::DataType::UNKNOWN;
                diags_.collect_diags(
                    "undefined index in table", name_left,
                    DiagnosticEngine::DiagType::ERROR, eNode   
                );
                return;
            }

            eNode->node_data_type = *dt;
        }
        else if (converted_ind_var->value.type == Type::IDENT) {
            eNode->node_data_type = Symbol::DataType::UNKNOWN;
        }
        else { 
            eNode->node_data_type = Symbol::DataType::UNKNOWN;
            diags_.collect_diags(
                "undefined index in table", name_left,
                DiagnosticEngine::DiagType::ERROR, eNode
            );
        }
    }
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
        VariableNode* converted_var = dynamic_cast<VariableNode*>(arg);
        EllipsisNode* converted_ell = dynamic_cast<EllipsisNode*>(arg);

        // this if stat can work only if param of func was invalid
        // so for expample: function foo(1+2, val) ... end 
        // here, its not allowed to pass '1+2' as func param
        if (converted_var) {
            std::string vName = std::get<std::string>(converted_var->value.value);
            Symbol vSymb = {
                .kind_ = Symbol::Kind::PARAM,
                .data_type_ = Symbol::DataType::UNKNOWN,
                .is_used_ = false, 
                .node_ = arg 
            };

            scopes_.back()->add_into_symbols(vName, vSymb);
        }
        // should change FunctionCallNode, MethodNode, MethodCallNode to support ellipsis
        else if (converted_ell) {
            fNode->have_ellipsis = true;
        }
        else {
            diags_.collect_diags(
                "invalid function param in function", funcName,
                DiagnosticEngine::DiagType::ERROR, fNode);
        }
    }

    for (const auto& var : fNode->body) {
        var->accept(*this);
    }
    removeScope();
    removeFuncScope();
}

void SemanticAnalyzer::visit(MethodNode* mNode) {
    std::string class_name = std::get<std::string>(mNode->className.value);
    Symbol* class_symb = scopes_.back()->lookup(class_name);

    if (!class_symb) {
        diags_.collect_diags(
            "undefined method parent", class_name,
            DiagnosticEngine::DiagType::ERROR, mNode);
        return;
    }

    std::string meth_name = std::get<std::string>(mNode->value.value);
    Symbol meth_symb = {
        .kind_ = (mNode->isLocal) ? Symbol::Kind::LOCAL : Symbol::Kind::GLOBAL,
        .data_type_ = Symbol::DataType::UNKNOWN,
        .name_ = meth_name,
        .is_used_ = false, 
        .node_ = mNode 
    };

    if (!class_symb->method_map_) {
        class_symb->method_map_.emplace();
    }

    class_symb->method_map_->push_back(meth_symb);

    makeFuncScope(mNode);
    makeScope();

    for (auto& arg : mNode->args) {
        VariableNode* converted = dynamic_cast<VariableNode*>(arg);

        if (!converted) {
            diags_.collect_diags(
                "invalid function param in function", meth_name,
                DiagnosticEngine::DiagType::ERROR, mNode);

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

    for (const auto& var : mNode->body) {
        var->accept(*this);
    }

    removeScope();
    removeFuncScope();
}

void SemanticAnalyzer::visit(AnonFunctionNode* afNode) {
    makeFuncScope(afNode);
    makeScope();

    if (!afNode->args.empty()) {
        for (auto& arg : afNode->args) {
            VariableNode* converted = dynamic_cast<VariableNode*>(arg);

            if (!converted) {
                diags_.collect_diags(
                    "invalid function param in", "anonymous function",
                    DiagnosticEngine::DiagType::ERROR, afNode);

                return;
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
    }

    for (const auto& var : afNode->body) {
        var->accept(*this);
    }

    removeScope();
    removeFuncScope();
}

void SemanticAnalyzer::visit(FunctionCallNode* fcNode) {
    for (const auto& a: fcNode->args) {
        a->accept(*this);
    }

    Symbol* callee_symb = getSymbolFromNode(fcNode->callee);
    if (!callee_symb) { return; }

    if (!callee_symb->return_types_.has_value() || callee_symb->return_types_->size() == 0) {
        fcNode->node_data_type = Symbol::DataType::NIL;
        fcNode->ret_data_types = std::vector<Symbol::DataType>{};
    } else {
        fcNode->node_data_type = callee_symb->return_types_.value()[0];
        fcNode->ret_data_types = callee_symb->return_types_.value();
    }
}

void SemanticAnalyzer::visit(MethodCallNode* mcNode) {
    for (const auto& a: mcNode->args) {
        a->accept(*this);
    }

    Symbol* symb = getSymbolFromNode(mcNode->object_name);
    if (!symb) { return; }

    if (!symb->method_map_.has_value()) {
        diags_.collect_diags(
                "use of undefined method", 
                std::get<std::string>(mcNode->method_name.value),
                DiagnosticEngine::DiagType::ERROR, mcNode);
        return;
    }

    Symbol* method = nullptr;
    std::string methNameStr = std::get<std::string>(mcNode->method_name.value);

    for (auto& name: symb->method_map_.value()) {
        if (methNameStr == name.name_) {
            method = &name;
            break;
        }
    }

    if (!method) {
        diags_.collect_diags(
                "use of undefined method", 
                std::get<std::string>(mcNode->method_name.value),
                DiagnosticEngine::DiagType::ERROR, mcNode);

        mcNode->node_data_type = Symbol::DataType::NIL;
        mcNode->return_types = std::vector<Symbol::DataType>{};
        return;
    }

    if (!method->return_types_.has_value() || method->return_types_->size() == 0) {
        mcNode->node_data_type = Symbol::DataType::NIL;
        mcNode->return_types = std::vector<Symbol::DataType>{};
    } else {
        mcNode->node_data_type = method->return_types_.value()[0];
        mcNode->return_types = method->return_types_.value();
    }
}

void SemanticAnalyzer::visit(ReturnNode* rNode) {
    std::vector<Symbol::DataType> temp_vect;

    for (const auto& iter : rNode->args) {
        iter->accept(*this);

        if (iter->node_data_type) {
            temp_vect.push_back(*iter->node_data_type);
        }
        else temp_vect.push_back(Symbol::DataType::NIL);
    }

    // if current function is a basic function (no matter local or global)
    if (std::holds_alternative<FunctionNode*>(currentFuncScope())) {
        FunctionNode* curr_func = std::get<FunctionNode*>(currentFuncScope());
        std::string fName = std::get<std::string>(curr_func->value.value);

        Symbol* symb = scopes_.back()->get_parent()->lookup(fName);
        if (!symb) {
            diags_.collect_diags(
                "function symbol not found in parent scope", 
                fName, DiagnosticEngine::DiagType::ERROR, rNode);
            return;
        }

        if (!symb->return_types_) {
            symb->return_types_ = temp_vect;
        }
        else {
            for (size_t i = 0; i < temp_vect.size(); ++i) {
                if (i >= symb->return_types_->size()) {
                    diags_.collect_diags(
                        "inconsistent/invalid return types in function", 
                        fName, DiagnosticEngine::DiagType::ERROR, rNode);
                    break;
                }

                if ((*symb->return_types_)[i] != temp_vect[i] && temp_vect[i] != Symbol::DataType::UNKNOWN) {
                    diags_.collect_diags(
                        "inconsistent/invalid return types in function", 
                        fName, DiagnosticEngine::DiagType::ERROR, rNode);
                } }
        }
    }

    // or the current is a anonymous function
    else if (std::holds_alternative<AnonFunctionNode*>(currentFuncScope())) {
        AnonFunctionNode* curr_func = std::get<AnonFunctionNode*>(currentFuncScope());
        curr_func->return_types = temp_vect;
    }

    // MethodNode
    else if (std::holds_alternative<MethodNode*>(currentFuncScope())) {
        MethodNode* curr_func = std::get<MethodNode*>(currentFuncScope());
        std::string fName = std::get<std::string>(curr_func->value.value);

        Symbol* class_symb = scopes_.back()->lookup(std::get<std::string>(curr_func->className.value));
        Symbol* symb = nullptr;

        if (class_symb && class_symb->method_map_.has_value()) {
            for (auto& it: class_symb->method_map_.value()) {
                if (it.name_ == fName) {
                    symb = &it;
                    break;
                }
            }
        }

        if (!symb) {
            diags_.collect_diags(
                "function symbol not found in parent scope", 
                fName, DiagnosticEngine::DiagType::ERROR, rNode);
            return;
        }

        if (!symb->return_types_) {
            symb->return_types_ = temp_vect;
        }
        else {
            for (size_t i = 0; i < temp_vect.size(); ++i) {
                if (i >= symb->return_types_->size()) {
                    diags_.collect_diags(
                        "inconsistent/invalid return types in function", 
                        fName, DiagnosticEngine::DiagType::ERROR, rNode);
                    break;
                }

                if ((*symb->return_types_)[i] != temp_vect[i] && temp_vect[i] != Symbol::DataType::UNKNOWN) {
                    diags_.collect_diags(
                        "inconsistent/invalid return types in function", 
                        fName, DiagnosticEngine::DiagType::ERROR, rNode);
                } }
        }
    }

    else {
        diags_.collect_diags(
            "compiler error", "failed to pass return arguments to function",
            DiagnosticEngine::DiagType::ERROR, rNode);
    }
}

void SemanticAnalyzer::visit(BinaryOpNode* boNode) {
    boNode->left->accept(*this);
    boNode->right->accept(*this);

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
                && boNode->left->node_data_type != Symbol::DataType::STRING
                && boNode->left->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
            }
            else if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::FLOAT
                && boNode->right->node_data_type != Symbol::DataType::STRING
                && boNode->right->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
            }

            boNode->node_data_type = Symbol::DataType::BOOL;
            break;

        case Type::PLUS:
        case Type::MINUS:
        case Type::STAR:
        case Type::PERCENT:
        case Type::DOUBLE_SLASH:
            if (boNode->left->node_data_type != Symbol::DataType::INT 
                && boNode->left->node_data_type != Symbol::DataType::FLOAT
                && boNode->left->node_data_type != Symbol::DataType::STRING
                && boNode->left->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
            }
            else if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::FLOAT
                && boNode->right->node_data_type != Symbol::DataType::STRING
                && boNode->right->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
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
                && boNode->left->node_data_type != Symbol::DataType::STRING
                && boNode->left->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
            }
            else if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::FLOAT
                && boNode->right->node_data_type != Symbol::DataType::STRING
                && boNode->right->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
            }

            boNode->node_data_type = Symbol::DataType::STRING;
            break;

        // in lua, a^b and a/b always return float
        case Type::CARET:
        case Type::SLASH:
            if (boNode->left->node_data_type != Symbol::DataType::INT 
                && boNode->left->node_data_type != Symbol::DataType::FLOAT
                && boNode->left->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
            }
            else if (boNode->right->node_data_type != Symbol::DataType::INT 
                && boNode->right->node_data_type != Symbol::DataType::STRING
                && boNode->right->node_data_type != Symbol::DataType::UNKNOWN) {
                diags_.collect_diags(
                    "invalid data type of", 
                    std::string(boNode->getName()),
                    DiagnosticEngine::DiagType::ERROR, boNode);
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

void SemanticAnalyzer::visit(IfNode* iNode) {
    iNode->condition->accept(*this);
    for (const auto& iter: iNode->elseifs) { iter->accept(*this); }
    for (const auto& iter: iNode->elseBody) { iter->accept(*this); }
    for (const auto& iter: iNode->body) { iter->accept(*this); }
}

void SemanticAnalyzer::visit(ElseIfNode* eiNode) { 
    eiNode->condition->accept(*this);
    for (const auto& iter: eiNode->body) { iter->accept(*this); }
}

void SemanticAnalyzer::visit(DoNode* dNode) {
    for (const auto& iter: dNode->body) { iter->accept(*this); }
}

void SemanticAnalyzer::visit(WhileNode* wNode) {
    wNode->condition->accept(*this);
    for (const auto& iter: wNode->body) { iter->accept(*this); }
}

void SemanticAnalyzer::visit(RepeatUntilNode* ruNode) {
    ruNode->condition->accept(*this);
    for (const auto& iter: ruNode->body) { iter->accept(*this); }
}

void SemanticAnalyzer::visit(NumericForNode* nfNode) { 
    makeScope();
    std::string iter_variable_name = std::get<std::string>(nfNode->var.value);
    Symbol iter_variable_symb = {
        .kind_ = Symbol::Kind::GLOBAL,
        .data_type_ = Symbol::DataType::UNKNOWN,
        .is_used_ = true, 
        .node_ = nfNode->start
    };

    scopes_.back()->add_into_symbols(iter_variable_name, iter_variable_symb);

    nfNode->start->accept(*this);
    nfNode->step->accept(*this);
    nfNode->finish->accept(*this);

    if (nfNode->finish->node_data_type != Symbol::DataType::BOOL) {
        diags_.collect_diags(
                "invalid for statement", 
                "finish block (the one between first and second ;) should be boolean",
                DiagnosticEngine::DiagType::ERROR, nfNode);
        return;
    }

    for (const auto& iter: nfNode->body) { iter->accept(*this); }
    removeScope();
}

void SemanticAnalyzer::visit(GenericForNode* gfNode) { 
    FunctionCallNode* converted_fn = dynamic_cast<FunctionCallNode*>(gfNode->fn);
    MethodCallNode* converted_mh = dynamic_cast<MethodCallNode*>(gfNode->fn);

    makeScope();
    if (converted_fn) {
        converted_fn->accept(*this);

        // too much variables
        if (converted_fn->ret_data_types->size() < gfNode->keyArgs.size()) {
            diags_.collect_diags(
                    "in 'generic for': wrong count of the varables or functions return value(s) statement with data type of", 
                    std::string(converted_fn->getName()), DiagnosticEngine::DiagType::ERROR, converted_fn);
            return;
        }

        for (size_t i = 0; i < gfNode->keyArgs.size(); ++i) {
            VariableNode* converted_key = dynamic_cast<VariableNode*>(gfNode->keyArgs[i]);
            if (!converted_key) {
                diags_.collect_diags(
                        "unexpected identificator with data type of", 
                        std::string(gfNode->keyArgs[i]->getName()),
                        DiagnosticEngine::DiagType::ERROR, gfNode->keyArgs[i]);
                return;
            }

            std::string name_key = std::get<std::string>(converted_key->value.value);

            Symbol symb = Symbol {
                .kind_ = Symbol::Kind::LOCAL,
                .data_type_ = Symbol::DataType::UNKNOWN,
                .is_used_ = false,
                .node_ = converted_key
            };

            scopes_.back()->add_into_symbols(name_key, symb);
        }
    }
    else if (converted_mh) {
        converted_mh->accept(*this);

        // too much variables
        if (converted_mh->return_types->size() < gfNode->keyArgs.size()) {
            diags_.collect_diags(
                    "in 'generic for': wrong count of the varables or methods return value(s) statement with data type of", 
                    std::string(converted_mh->getName()), DiagnosticEngine::DiagType::ERROR, converted_mh);
            return;
        }

        for (size_t i = 0; i < gfNode->keyArgs.size(); ++i) {
            VariableNode* converted_key = dynamic_cast<VariableNode*>(gfNode->keyArgs[i]);
            if (!converted_key) {
                diags_.collect_diags(
                        "unexpected identificator with data type of", 
                        std::string(gfNode->keyArgs[i]->getName()),
                        DiagnosticEngine::DiagType::ERROR, gfNode->keyArgs[i]);
                return;
            }

            std::string name_key = std::get<std::string>(converted_key->value.value);
            Symbol symb = Symbol {
                .kind_ = Symbol::Kind::LOCAL,
                .data_type_ = Symbol::DataType::UNKNOWN,
                .is_used_ = false,
                .node_ = converted_key
            };

            scopes_.back()->add_into_symbols(name_key, symb);
        }
    }
    else {
        diags_.collect_diags(
                "unexpected identificator with data type of",
                std::string(gfNode->fn->getName()),
                DiagnosticEngine::DiagType::ERROR, gfNode->fn);
        return;
    }

    for (const auto& iter: gfNode->body) { iter->accept(*this); }
    removeScope();
}
