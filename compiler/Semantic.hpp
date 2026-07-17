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
    EnumConstant,
    Struct,
    Union,
    Enum
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
    AttributeList attributes;
    bool is_used = false;
    int scope_depth = 0;

    std::vector<StructMember> members;
    int size = 0;
    int alignment = 0;
    std::vector<std::pair<std::string, int>> enum_constants;

    bool hasAttribute(const std::string& name) const {
        for (const auto& attr : attributes) {
            if (attr.name == name) return true;
        }
        return false;
    }

    std::string getAttributeArg(const std::string& name) const {
        for (const auto& attr : attributes) {
            if (attr.name == name) return attr.argument;
        }
        return "";
    }
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
    const std::unordered_map<std::string, std::unique_ptr<Symbol>>& getSymbols() const {
        return symbols;
    }
};
}   // namespace dix
#endif // SEMANTIC_HPP