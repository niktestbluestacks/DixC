// dix
#include <SemanticAnalyzer.hpp>
#include <memory>

#include "Ast.hpp"
#include "Semantic.hpp"

namespace dix {
const std::set<std::string> SemanticAnalyzer::ValidAttributes = {
    "nodiscard",    "noreturn", "deprecated", "fallthrough",
    "maybe_unused", "likely",   "unlikely"};

// Scope namadgement

void SemanticAnalyzer::enterScope(const std::string& name) {
  owned_scopes.push_back(std::make_unique<Scope>(current_scope, name));
  current_scope = owned_scopes.back().get();
}

void SemanticAnalyzer::exitScope() {
  if (!owned_scopes.empty()) {
    owned_scopes.pop_back();
    current_scope =
        owned_scopes.empty() ? &global_scope : owned_scopes.back().get();
  }
}

// Entrypoint

std::vector<Diagnostic> SemanticAnalyzer::analyze(Statement* program) {
  diagnostics.clear();
  if (program) {
    program->accept(*this);
  }
  return diagnostics;
}

// Attribute validation

void SemanticAnalyzer::validateAttributes(const AttributeList& attrs, int line,
                                          int col, const std::string& context) {
  for (const auto& attr : attrs) {
    if (ValidAttributes.find(attr.name) == ValidAttributes.end()) {
      error(line, col, "Unknown attribute '" + attr.name + "'");
      continue;
    }
    if (attr.name == "nodiscard") {
      if (context.find("variable") != std::string::npos ||
          context.find("parameter") != std::string::npos) {
        error(line, col, "'[[nodiscard]]' cannot be applied to " + context);
      }
    } else if (attr.name == "noreturn") {
      if (context.find("function") == std::string::npos) {
        error(line, col, "'[[noreturn]]' can only be applied to functions");
      }
      if (!attr.argument.empty())
        error(line, col, "'[[noreturn]]' attribute takes no arguments");
    } else if (attr.name == "fallthrough" || attr.name == "maybe_unused" ||
               attr.name == "likely" || attr.name == "unlikely") {
      if (!attr.argument.empty()) {
        error(line, col,
              "'[[" + attr.name + "]]' attribute takes no arguments");
      }
      if (attr.name == "likely" || attr.name == "unlikely") {
        bool valid_ctx = (context.find("if statement") != std::string::npos ||
                          context.find("case label") != std::string::npos ||
                          context.find("default label") != std::string::npos);
        if (!valid_ctx)
          error(line, col,
                "'[[" + attr.name +
                    "]]' can only be applied to if statements and case/default "
                    "labels");
      }
    }
  }
}

void SemanticAnalyzer::visit(BlockStatement* stmt) {
  enterScope("block");
  for (auto& s : stmt->statements) s->accept(*this);

  for (const auto& [name, sym] : current_scope->getSymbols()) {
    if (sym->kind == SymbolKind::Variable && !sym->is_used &&
        !sym->hasAttribute("maybe_unused")) {
      warning(sym->line, sym->column, "Unused variable '" + name + "'");
    }
  }
  exitScope();
}

void SemanticAnalyzer::visit(VarDeclaration* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column,
                     "variable '" + stmt->name + "'");

  auto symbol = std::make_unique<Symbol>();
  symbol->name = stmt->name;
  symbol->kind = SymbolKind::Variable;
  symbol->type = cloneType(stmt->type.get());
  symbol->line = stmt->line;
  symbol->column = stmt->column;
  symbol->is_initialized = (stmt->initializer != nullptr);
  symbol->is_constexpr = stmt->is_constexpr;
  symbol->attributes = stmt->attributes;

  if (!current_scope->defineOrdinary(std::move(symbol))) {
    error(stmt->line, stmt->column, "Redefinition of '" + stmt->name + "'");
  }

  if (stmt->initializer) {
    stmt->initializer->accept(*this);
    const Type* var_type = stmt->type.get();
    const Type* init_type = stmt->initializer->resolved_type.get();
    if (!typesCompatible(init_type, var_type)) {
      error(stmt->line, stmt->column,
            "Incompatible types in initialization of '" + stmt->name + "'");
    }
  }
}

void SemanticAnalyzer::visit(FuncDeclaration* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column,
                     "function '" + stmt->name + "'");

  for (const auto& attr : stmt->attributes) {
    if (attr.name == "noreturn") {
      if (auto* basic =
              dynamic_cast<const BasicType*>(stmt->return_type.get())) {
        if (basic->kind != BasicType::Kind::Void) {
          error(stmt->line, stmt->column,
                "[[noreturn]] function '" + stmt->name +
                    "' must have void return type");
        }
      }
    }
  }

  auto symbol = std::make_unique<Symbol>();
  symbol->name = stmt->name;
  symbol->kind = SymbolKind::Function;
  symbol->type = cloneType(stmt->return_type.get());
  symbol->line = stmt->line;
  symbol->column = stmt->column;
  symbol->is_defined = (stmt->body != nullptr);
  for (auto& param : stmt->parameters)
    symbol->param_types.push_back(cloneType(param.type.get()));

  Symbol* existing = current_scope->resolveOrdinary(stmt->name);
  if (existing) {
    if (existing->kind != SymbolKind::Function) {
      error(stmt->line, stmt->column,
            "'" + stmt->name + "' redeclared as different kind of symbol");
    } else if (stmt->body && existing->is_defined) {
      error(stmt->line, stmt->column,
            "Redefinition of function '" + stmt->name + "'");
    } else {
      existing->is_defined = existing->is_defined || (stmt->body != nullptr);
    }
  } else {
    current_scope->defineOrdinary(std::move(symbol));
  }

  if (stmt->body) {
    const FuncDeclaration* old_func = current_function;
    current_function = stmt;
    bool old_has_return = has_return;
    has_return = false;

    enterScope("function:" + stmt->name);
    for (auto& param : stmt->parameters) {
      auto param_sym = std::make_unique<Symbol>();
      param_sym->name = param.name;
      param_sym->kind = SymbolKind::Parameter;
      param_sym->type = cloneType(param.type.get());
      param_sym->line = stmt->line;
      param_sym->column = stmt->column;
      if (!current_scope->defineOrdinary(std::move(param_sym))) {
        error(stmt->line, stmt->column,
              "Duplicate parameter name '" + param.name + "'");
      }
    }

    stmt->body->accept(*this);

    if (!has_return) {
      if (auto* basic =
              dynamic_cast<const BasicType*>(stmt->return_type.get())) {
        if (basic->kind != BasicType::Kind::Void && stmt->name != "main") {
          Symbol* func_sym = current_scope->resolveOrdinary(stmt->name);
          if (!func_sym || !func_sym->hasAttribute("noreturn")) {
            error(
                stmt->line, stmt->column,
                "Non-void function '" + stmt->name + "' should return a value");
          }
        }
      }
    }
    exitScope();
    current_function = old_func;
    has_return = old_has_return;
  }
}

void SemanticAnalyzer::visit(ExpressionStatement* stmt) {
  if (stmt->expr) {
    stmt->expr->accept(*this);
    if (auto* call = dynamic_cast<CallExpr*>(stmt->expr.get())) {
      if (auto* ident = dynamic_cast<IdentifierExpr*>(call->callee.get())) {
        Symbol* sym = current_scope->resolveOrdinary(ident->name);
        if (sym && sym->hasAttribute("nodiscard")) {
          std::string msg = "Ignoring return value of function '" +
                            ident->name + "' declared with [[nodiscard]]";
          std::string reason = sym->getAttributeArg("nodiscard");
          if (!reason.empty()) msg += ": " + reason;
          error(call->line, call->column, msg);
        }
      }
    }
  }
}

void SemanticAnalyzer::visit(ReturnStatement* stmt) {
  if (!current_function) {
    error(stmt->line, stmt->column, "'return' outside of function");
    return;
  }
  Symbol* func_sym = current_scope->resolveOrdinary(current_function->name);
  if (func_sym && func_sym->hasAttribute("noreturn")) {
    error(stmt->line, stmt->column,
          "Function '" + current_function->name +
              "' declared [[noreturn]] should not return");
  }

  has_return = true;
  if (stmt->expr) {
    stmt->expr->accept(*this);
    const Type* func_ret_type = current_function->return_type.get();
    const Type* expr_type = stmt->expr->resolved_type.get();
    if (auto* basic = dynamic_cast<const BasicType*>(func_ret_type)) {
      if (basic->kind == BasicType::Kind::Void) {
        error(stmt->line, stmt->column,
              "void function should not return a value");
      } else if (!typesCompatible(expr_type, func_ret_type)) {
        error(stmt->line, stmt->column,
              "Incompatible types in return statement");
      }
    }
  } else {
    if (current_function->name != "main") {
      if (auto* basic = dynamic_cast<const BasicType*>(
              current_function->return_type.get())) {
        if (basic->kind != BasicType::Kind::Void) {
          error(stmt->line, stmt->column,
                "Non-void function '" + current_function->name +
                    "' should return a value");
        }
      }
    }
  }
}

void SemanticAnalyzer::visit(IfStatement* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column,
                     "if statement");
  stmt->condition->accept(*this);
  stmt->thenBranch->accept(*this);
  if (stmt->elseBranch) stmt->elseBranch->accept(*this);
}

void SemanticAnalyzer::visit(ElseStatement* stmt) {
  if (stmt->body) stmt->body->accept(*this);
}

void SemanticAnalyzer::visit(WhileStatement* stmt) {
  stmt->condition->accept(*this);
  loop_depth++;
  stmt->body->accept(*this);
  loop_depth--;
}

void SemanticAnalyzer::visit(ForStatement* stmt) {
  enterScope("for");
  if (stmt->init) stmt->init->accept(*this);
  if (stmt->condition) stmt->condition->accept(*this);
  if (stmt->increment) stmt->increment->accept(*this);
  loop_depth++;
  stmt->body->accept(*this);
  loop_depth--;
  exitScope();
}

void SemanticAnalyzer::visit(SwitchStatement* stmt) {
  stmt->condition->accept(*this);
  const Type* cond_type = stmt->condition->resolved_type.get();
  if (!isNumericType(cond_type))
    error(stmt->line, stmt->column, "Switch condition must be an integer type");

  switch_depth++;
  stmt->body->accept(*this);
  switch_depth--;
}

void SemanticAnalyzer::visit(CaseStatement* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column, "case label");
  stmt->value->accept(*this);
  const Type* val_type = stmt->value->resolved_type.get();
  if (!isNumericType(val_type))
    error(stmt->line, stmt->column, "Case value must be an integer constant");
  if (stmt->body) stmt->body->accept(*this);
}

void SemanticAnalyzer::visit(DefaultStatement* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column,
                     "default label");
  if (stmt->body) stmt->body->accept(*this);
}

void SemanticAnalyzer::visit(BreakStatement* stmt) {
  if (loop_depth == 0 && switch_depth == 0)
    error(stmt->line, stmt->column,
          "'break' statement not within loop or switch");
}

void SemanticAnalyzer::visit(ContinueStatement* stmt) {
  if (loop_depth == 0)
    error(stmt->line, stmt->column, "'continue' statement not within a loop");
}

void SemanticAnalyzer::visit(AttributeStatement* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column, "statement");
  for (const auto& attr : stmt->attributes) {
    if (attr.name == "fallthrough" && switch_depth == 0) {
      error(stmt->line, stmt->column,
            "'[[fallthrough]]' statement not within a switch");
    }
  }
}

void SemanticAnalyzer::visit(StructDecl* stmt) {
  std::string context = stmt->is_union ? "union '" + stmt->name + "'"
                                       : "struct '" + stmt->name + "'";
  validateAttributes(stmt->attributes, stmt->line, stmt->column, context);

  if (!stmt->name.empty()) {
    auto symbol = std::make_unique<Symbol>();
    symbol->name = stmt->name;
    symbol->kind = stmt->is_union ? SymbolKind::Union : SymbolKind::Struct;
    symbol->line = stmt->line;
    symbol->column = stmt->column;
    symbol->is_defined = !stmt->members.empty();

    // Note: C allows redefining structs if they are identical, but for
    // simplicity:
    if (!current_scope->defineTag(std::move(symbol))) {
      // error(stmt->line, stmt->column, "Redefinition of " + context);
    }
  }

  // Check for duplicate member names within the struct/union
  std::set<std::string> member_names;
  for (const auto& member : stmt->members) {
    if (member_names.count(member.name)) {
      error(member.line, member.column,
            "Duplicate member '" + member.name + "' in " + context);
    }
    member_names.insert(member.name);
  }
}

void SemanticAnalyzer::visit(EnumDecl* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column,
                     "enum '" + stmt->name + "'");

  if (!stmt->name.empty()) {
    auto symbol = std::make_unique<Symbol>();
    symbol->name = stmt->name;
    symbol->kind = SymbolKind::Enum;
    symbol->line = stmt->line;
    symbol->column = stmt->column;
    symbol->is_defined = !stmt->constants.empty();
    current_scope->defineTag(std::move(symbol));
  }

  for (const auto& constant : stmt->constants) {
    auto const_sym = std::make_unique<Symbol>();
    const_sym->name = constant.name;
    const_sym->kind = SymbolKind::EnumConstant;
    const_sym->line = constant.line;
    const_sym->column = constant.column;
    const_sym->is_constexpr = true;

    if (!current_scope->defineOrdinary(std::move(const_sym))) {
      error(constant.line, constant.column,
            "Redefinition of enumerator '" + constant.name + "'");
    }
  }
}

void SemanticAnalyzer::visit(TypedefDecl* stmt) {
  validateAttributes(stmt->attributes, stmt->line, stmt->column,
                     "typedef '" + stmt->name + "'");

  auto symbol = std::make_unique<Symbol>();
  symbol->name = stmt->name;
  symbol->kind = SymbolKind::Typedef;
  symbol->type = std::move(stmt->underlying_type);
  symbol->line = stmt->line;
  symbol->column = stmt->column;

  if (!current_scope->defineOrdinary(std::move(symbol))) {
    error(stmt->line, stmt->column,
          "Redefinition of typedef '" + stmt->name + "'");
  }
}

void SemanticAnalyzer::visit(NumberExpr* expr) {
  auto type = std::make_unique<BasicType>();
  type->kind = expr->is_float ? BasicType::Kind::Double : BasicType::Kind::Int;
  expr->resolved_type = std::move(type);
}

void SemanticAnalyzer::visit(IdentifierExpr* expr) {
  Symbol* sym = current_scope->resolveOrdinary(expr->name);
  if (!sym) {
    error(expr->line, expr->column,
          "Use of undeclared identifier '" + expr->name + "'");
  } else {
    sym->is_used = true;
    expr->resolved_type = cloneType(sym->type.get());
    if (sym->hasAttribute("deprecated")) {
      std::string msg = "'" + expr->name + "' is deprecated";
      std::string reason = sym->getAttributeArg("deprecated");
      if (!reason.empty()) msg += ": " + reason;
      warning(expr->line, expr->column, msg);
    }
  }
}

void SemanticAnalyzer::visit(BinaryExpr* expr) {
  expr->left->accept(*this);
  expr->right->accept(*this);
  const Type* left_type = expr->left->resolved_type.get();
  const Type* right_type = expr->right->resolved_type.get();
  TokenType op = expr->op;

  if (op == TokenType::Plus || op == TokenType::Minus ||
      op == TokenType::Star || op == TokenType::Slash ||
      op == TokenType::Percent) {
    if (!isNumericType(left_type) || !isNumericType(right_type))
      error(expr->line, expr->column, "Invalid operands to binary expression");
    if (op == TokenType::Plus || op == TokenType::Minus) {
      if (isPointerType(left_type) && isNumericType(right_type))
        expr->resolved_type = cloneType(left_type);
      else if (isNumericType(left_type) && isPointerType(right_type) &&
               op == TokenType::Plus)
        expr->resolved_type = cloneType(right_type);
      else if (isPointerType(left_type) && isPointerType(right_type) &&
               op == TokenType::Minus) {
        auto type = std::make_unique<BasicType>();
        type->kind = BasicType::Kind::Long;
        expr->resolved_type = std::move(type);
      } else
        expr->resolved_type = getCommonType(left_type, right_type);
    } else
      expr->resolved_type = getCommonType(left_type, right_type);
  } else if (op == TokenType::Less || op == TokenType::Greater ||
             op == TokenType::LessEqual || op == TokenType::GreaterEqual ||
             op == TokenType::EqualEqual || op == TokenType::ExclaimEqual) {
    if (!isNumericType(left_type) && !isPointerType(left_type))
      error(expr->line, expr->column, "Invalid left operand to comparison");
    if (!isNumericType(right_type) && !isPointerType(right_type))
      error(expr->line, expr->column, "Invalid right operand to comparison");
    auto type = std::make_unique<BasicType>();
    type->kind = BasicType::Kind::Int;
    expr->resolved_type = std::move(type);
  } else if (op == TokenType::AmpAmp || op == TokenType::PipePipe) {
    auto type = std::make_unique<BasicType>();
    type->kind = BasicType::Kind::Int;
    expr->resolved_type = std::move(type);
  } else if (op == TokenType::Ampersand || op == TokenType::Pipe ||
             op == TokenType::Caret || op == TokenType::LShift ||
             op == TokenType::RShift) {
    if (!isNumericType(left_type) || !isNumericType(right_type))
      error(expr->line, expr->column, "Invalid operands to bitwise expression");
    expr->resolved_type = getCommonType(left_type, right_type);
  } else if (op == TokenType::Equal || op == TokenType::PlusEqual ||
             op == TokenType::MinusEqual || op == TokenType::StarEqual ||
             op == TokenType::SlashEqual || op == TokenType::PercentEqual) {
    if (!isLvalue(expr->left.get()))
      error(expr->line, expr->column, "Expression is not assignable");
    if (auto* ident = dynamic_cast<IdentifierExpr*>(expr->left.get())) {
      Symbol* sym = current_scope->resolveOrdinary(ident->name);
      if (sym && sym->type) {
        if (auto* basic = dynamic_cast<const BasicType*>(sym->type.get())) {
          if (basic->is_const)
            error(expr->line, expr->column,
                  "Cannot assign to const variable '" + ident->name + "'");
        }
      }
    }
    if (!typesCompatible(right_type, left_type))
      error(expr->line, expr->column, "Incompatible types in assignment");
    expr->resolved_type = cloneType(left_type);
  }
}

void SemanticAnalyzer::visit(UnaryExpr* expr) {
  expr->operand->accept(*this);
  const Type* operand_type = expr->operand->resolved_type.get();
  if (expr->op == TokenType::Minus || expr->op == TokenType::Plus) {
    if (!isNumericType(operand_type))
      error(expr->line, expr->column, "Invalid operand to unary expression");
    expr->resolved_type = cloneType(operand_type);
  } else if (expr->op == TokenType::Exclaim) {
    auto type = std::make_unique<BasicType>();
    type->kind = BasicType::Kind::Int;
    expr->resolved_type = std::move(type);
  } else if (expr->op == TokenType::Tilde) {
    if (!isNumericType(operand_type))
      error(expr->line, expr->column, "Invalid operand to bitwise not");
    expr->resolved_type = cloneType(operand_type);
  } else if (expr->op == TokenType::Star) {
    if (auto* ptr_type = dynamic_cast<const PointerType*>(operand_type))
      expr->resolved_type = cloneType(ptr_type->base_type.get());
    else
      error(expr->line, expr->column, "Dereference of non-pointer type");
  } else if (expr->op == TokenType::Ampersand) {
    auto ptr_type = std::make_unique<PointerType>();
    ptr_type->base_type = cloneType(operand_type);
    expr->resolved_type = std::move(ptr_type);
  }
}

void SemanticAnalyzer::visit(CallExpr* expr) {
  expr->callee->accept(*this);
  for (auto& arg : expr->arguments) arg->accept(*this);

  if (auto* ident = dynamic_cast<IdentifierExpr*>(expr->callee.get())) {
    Symbol* sym = current_scope->resolveOrdinary(ident->name);
    if (sym && sym->kind == SymbolKind::Function) {
      size_t expected = sym->param_types.size();
      size_t actual = expr->arguments.size();
      if (actual < expected)
        error(expr->line, expr->column,
              "Too few arguments to function '" + ident->name + "'");
      else if (actual > expected)
        error(expr->line, expr->column,
              "Too many arguments to function '" + ident->name + "'");

      for (size_t i = 0; i < std::min(actual, expected); i++) {
        if (!typesCompatible(expr->arguments[i]->resolved_type.get(),
                             sym->param_types[i].get())) {
          error(expr->arguments[i]->line, expr->arguments[i]->column,
                "Incompatible type for argument " + std::to_string(i + 1));
        }
      }
      expr->resolved_type = cloneType(sym->type.get());
    } else if (sym && sym->kind != SymbolKind::Function) {
      error(expr->line, expr->column,
            "Called object '" + ident->name + "' is not a function");
    }
  }
}

void SemanticAnalyzer::visit(IndexExpr* expr) {
  expr->array->accept(*this);
  expr->index->accept(*this);
  const Type* array_type = expr->array->resolved_type.get();
  const Type* index_type = expr->index->resolved_type.get();

  if (auto* arr_type = dynamic_cast<const ArrayType*>(array_type))
    expr->resolved_type = cloneType(arr_type->element_type.get());
  else if (auto* ptr_type = dynamic_cast<const PointerType*>(array_type))
    expr->resolved_type = cloneType(ptr_type->base_type.get());
  else
    error(expr->line, expr->column,
          "Subscripted value is not an array or pointer");

  if (!isNumericType(index_type))
    error(expr->line, expr->column, "Array subscript is not an integer");
}

void SemanticAnalyzer::visit(ConditionalExpr* expr) {
  expr->condition->accept(*this);
  expr->trueExpr->accept(*this);
  expr->falseExpr->accept(*this);
  expr->resolved_type = getCommonType(expr->trueExpr->resolved_type.get(),
                                      expr->falseExpr->resolved_type.get());
}

void SemanticAnalyzer::visit(PostfixExpr* expr) {
  expr->operand->accept(*this);
  const Type* operand_type = expr->operand->resolved_type.get();
  if (!isNumericType(operand_type) && !isPointerType(operand_type))
    error(expr->line, expr->column, "Invalid operand to increment/decrement");
  expr->resolved_type = cloneType(operand_type);
}

void SemanticAnalyzer::visit(MemberExpr* expr) {
  expr->object->accept(*this);
  const Type* obj_type = expr->object->resolved_type.get();

  if (expr->is_arrow) {
    if (auto* ptr_type = dynamic_cast<const PointerType*>(obj_type)) {
      obj_type = ptr_type->base_type.get();
    } else {
      error(expr->line, expr->column, "Member access through non-pointer type");
      return;
    }
  }

  obj_type = resolveTypedef(obj_type);

  if (auto* struct_type = dynamic_cast<const StructType*>(obj_type)) {
    bool found = false;
    for (const auto& member : struct_type->members) {
      if (member.name == expr->member) {
        expr->resolved_type = cloneType(member.type.get());
        found = true;
        break;
      }
    }
    if (!found) {
      error(expr->line, expr->column,
            "No member named '" + expr->member + "' in '" +
                (struct_type->is_union ? "union " : "struct ") +
                struct_type->name + "'");
    }
  } else {
    error(expr->line, expr->column,
          "Member access on non-struct/non-union type");
  }
}

void SemanticAnalyzer::visit(ParenExpr* expr) {
  expr->inner->accept(*this);
  expr->resolved_type = cloneType(expr->inner->resolved_type.get());
}

void SemanticAnalyzer::visit(StringExpr* expr) {
  auto ptr_type = std::make_unique<PointerType>();
  auto char_type = std::make_unique<BasicType>();
  char_type->kind = BasicType::Kind::Char;
  char_type->is_const = true;
  ptr_type->base_type = std::move(char_type);
  expr->resolved_type = std::move(ptr_type);
}

void SemanticAnalyzer::visit(CharExpr* expr) {
  auto type = std::make_unique<BasicType>();
  type->kind = BasicType::Kind::Int;
  expr->resolved_type = std::move(type);
}

bool SemanticAnalyzer::isLvalue(Expr* expr) {
  if (!expr) return false;
  if (dynamic_cast<IdentifierExpr*>(expr)) return true;
  if (auto* unary = dynamic_cast<UnaryExpr*>(expr))
    if (unary->op == TokenType::Star) return true;
  if (dynamic_cast<IndexExpr*>(expr)) return true;
  if (dynamic_cast<MemberExpr*>(expr)) return true;
  if (auto* paren = dynamic_cast<ParenExpr*>(expr))
    return isLvalue(paren->inner.get());
  return false;
}

std::string SemanticAnalyzer::typeToString(const Type* type) {
  if (!type) return "unknown";
  if (auto* basic = dynamic_cast<const BasicType*>(type)) {
    std::string result;
    if (basic->is_const) result += "const ";
    if (basic->is_unsigned) result += "unsigned ";
    switch (basic->kind) {
      case BasicType::Kind::Void:
        result += "void";
        break;
      case BasicType::Kind::Bool:
        result += "bool";
        break;
      case BasicType::Kind::Char:
        result += "char";
        break;
      case BasicType::Kind::Short:
        result += "short";
        break;
      case BasicType::Kind::Int:
        result += "int";
        break;
      case BasicType::Kind::Long:
        result += "long";
        break;
      case BasicType::Kind::Float:
        result += "float";
        break;
      case BasicType::Kind::Double:
        result += "double";
        break;
    }
    return result;
  }
  if (auto* ptr = dynamic_cast<const PointerType*>(type))
    return typeToString(ptr->base_type.get()) + "*";
  if (auto* arr = dynamic_cast<const ArrayType*>(type))
    return typeToString(arr->element_type.get()) + "[]";
  return "unknown";
}

}  // namespace dix