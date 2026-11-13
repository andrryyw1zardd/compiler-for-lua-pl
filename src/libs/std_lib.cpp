#include "libs/std_lib.hpp"

void STANDARD::initAll(SemanticAnalyzer* sa) {
    STANDARD::initGlobalFunctions(sa);
    STANDARD::initGlobalMethods(sa);
}

void STANDARD::initGlobalFunctions(SemanticAnalyzer* sa) {
    for (size_t i = 0; i < STANDARD::globalFuncRetTypes.size(); ++i) {
        Symbol symb = {
            .kind_ = Symbol::Kind::GLOBAL,
            .data_type_ = Symbol::DataType::NIL,
            .return_types_ = STANDARD::globalFuncRetTypes[i],
            .is_used_ = false,
            .node_ = nullptr };

        sa->scopes_.front()->add_into_symbols(STANDARD::globalFuncNames[i], symb);
    }
}

void STANDARD::initGlobalMethods(SemanticAnalyzer* sa) {
    for (size_t i = 0; i < STANDARD::globalMethodRetTypes.size(); ++i) {
        Symbol parent_symb = {
            .kind_ = Symbol::Kind::GLOBAL,
            .data_type_ = Symbol::DataType::NIL,
            .name_ = STANDARD::globalIdentNames[i],
            .is_used_ = false,
            .node_ = nullptr 
        };

        parent_symb.method_map_.emplace();
        parent_symb.method_map_->reserve(STANDARD::globalMethodNames[i].size());

        for (size_t j = 0; j < STANDARD::globalMethodNames[i].size(); ++j) {
            Symbol method_symb = {
                .kind_ = Symbol::Kind::LOCAL,
                .data_type_ = STANDARD::globalMethodRetTypes[i][j],
                .name_ = STANDARD::globalMethodNames[i][j],
            };

            method_symb.return_types_.emplace();
            method_symb.return_types_->push_back(STANDARD::globalMethodRetTypes[i][j]);

            parent_symb.method_map_->push_back(method_symb);
        }

        sa->scopes_.front()->add_into_symbols(STANDARD::globalIdentNames[i], parent_symb);
    }
}
