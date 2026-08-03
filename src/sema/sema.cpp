#include "sema/sema.hpp"

void ScopeStack::add_into_symbols(const std::string& name, const Symbol& symbol) {
    symbols_[name] = symbol;
}

Symbol* ScopeStack::lookup(const std::string& name) {
    auto itValue = symbols_.find(name);

    if (itValue != symbols_.end()) {
        return &itValue->second;
    }

    if (parent_) {
        return parent_->lookup(name); 
    }

    return nullptr;
}

bool ScopeStack::has_locally(const std::string& name) const {
    return symbols_.find(name) != symbols_.end();
}
