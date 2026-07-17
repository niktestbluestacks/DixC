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
  std::unordered_map<std::string, std::unique_ptr<Symbol>> ordinary;
  std::unordered_map<std::string, std::unique_ptr<Symbol>> tags;
  Scope* parent;
  std::string name;  // for debugging and maybe, maybe __func__
 public:
  Scope(Scope* parent, std::string name)
      : parent{parent}, name{std::move(name)} {}

  bool defineOrdinary(std::unique_ptr<Symbol> symbol) {
    const std::string& name = symbol->name;
    if (ordinary.find(name) != ordinary.end()) {
      return false;
    }
    ordinary[name] = std::move(symbol);
    return true;
  }

  Symbol* resolveOrdinary(const std::string& name) {
    auto it = ordinary.find(name);
    if (it != ordinary.end()) {
      return it->second.get();
    }
    if (parent) {
      return parent->resolveOrdinary(name);
    }
    return nullptr;
  }

  bool defineTag(std::unique_ptr<Symbol> symbol) {
    const std::string& name = symbol->name;
    if (tags.find(name) != tags.end()) {
      return false;
    }
    tags[name] = std::move(symbol);
    return true;
  }

  Symbol* resolveTag(const std::string& name) {
    auto it = tags.find(name);
    if (it != tags.end()) {
      return it->second.get();
    }
    if (parent) {
      return parent->resolveTag(name);
    }
    return nullptr;
  }

  bool containsLocalOrdinary(const std::string& name) const {
    return ordinary.find(name) != ordinary.end();
  }

  bool containsLocalTag(const std::string& name) const {
    return tags.find(name) != tags.end();
  }

  Scope* getParent() { return parent; }
  const std::string& getName() const { return name; }

  const std::unordered_map<std::string, std::unique_ptr<Symbol>>& getSymbols()
      const {
    return ordinary;
  }
};
}  // namespace dix
#endif  // SEMANTIC_HPP