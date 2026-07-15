#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

// dix
#include <Ast.hpp>

/// std
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace dix {
enum class SymbolKind {
    Variable,
    Function,
    Parameter,
    Typedef,
    EnumConstant
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    std::unique_ptr<Type> type;
    int line;
    int column;

    bool is_defined = false;
    std::vector<std::unique_ptr<Type>> param_types;
    bool is_initialized = false;
    bool is_constexpr = false;
};

class Scope {
private:
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    Scope* parent;
    std::string name;   // for debugging and maybe, maybe __func__
public:
    Scope(Scope* parent, std::string name):
        parent{parent},
        name{std::move(name)} {}

    bool define(std::unique_ptr<Symbol> symbol) {
        const std::string& name = symbol->name;
        if (symbols.find(name) != symbols.end()) {
            return false;
        }
        symbols[name] = std::move(symbol);
        return true;
    }

    Symbol* resolve(const std::string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return it->second.get();
        }
        if (parent) {
            return parent->resolve(name);
        }
        return nullptr;
    }

    bool containsLocal(const std::string& name) const {
        return symbols.find(name) != symbols.end();
    }

    Scope* getParent() { return parent; }
    const std::string& getName() const { return name; }
};
}   // namespace dix
#endif // SEMANTIC_HPP