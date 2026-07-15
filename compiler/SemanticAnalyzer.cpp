// dix
#include "Ast.hpp"
#include <SemanticAnalyzer.hpp>

namespace dix {
const std::set<std::string> SemanticAnalyzer::ValidAttributes = {
    "nodiscard",
    "noreturn", 
    "deprecated",
    "fallthrough",
    "maybe_unused",
    "likely",
    "unlikely"
};

std::vector<Diagnostic> SemanticAnalyzer::analyze(Statement* program) {
    diagnostics.clear();
    current_scope = &global_scope;
    
    visitStatement(program);
    
    return diagnostics;
}

void SemanticAnalyzer::validateAttributes(const AttributeList& attrs,
        int line, int col, const std::string& context) {
    for (const auto& attr : attrs) {
        if (ValidAttributes.find(attr.name) == ValidAttributes.end()) {
            error(line, col, "Unknown attribute '" + attr.name + "'");
            continue;
        }

        if (attr.name == "nodiscard") {
            if (context.find("variable") != std::string::npos ||
                context.find("parameter") != std::string::npos) {
                error(line, col, 
                    "'[[nodiscard]]' cannot be applied to " + context);
            }
        } 
        else if (attr.name == "noreturn") {
            if (context.find("function") == std::string::npos) {
                error(line, col, 
                    "'[[noreturn]]' can only be applied to functions");
                return;
            }

            if (!attr.argument.empty()) {
                error(line, col, 
                    "'[[noreturn]]' attribute takes no arguments");
            }
        }
        else if (attr.name == "deprcated") {
            // NO validation needed. can appear anywhere
        }
        else if (attr.name == "fallthrough") {
            if (!attr.argument.empty()) {
                error(line, col, 
                "'[[fallthrough]]' attribute takes no arguments");
            }
        }
        else if (attr.name == "maybe_unused") {
            if (!attr.argument.empty()) {
                error(line, col, 
                    "'[[maybe_unused]] attribute takes no arguments");
            }
        }
        else if (attr.name == "likely") {
            bool valid_context = 
                (context.find("if statement") != std::string::npos) ||
                (context.find("case label") != std::string::npos) ||
                (context.find("default label") != std::string::npos);
            
            if (!valid_context) {
                error(line, col, 
                    "'[[likely]]' can only be applied to if statements and case/default labels");
            }
            
            if (!attr.argument.empty()) {
                error(line, col, "'[[likely]]' attribute takes no arguments");
            }
        }
        else if (attr.name == "unlikely") {
            bool valid_context = 
                (context.find("if statement") != std::string::npos) ||
                (context.find("case label") != std::string::npos) ||
                (context.find("default label") != std::string::npos);
            
            if (!valid_context) {
                error(line, col, 
                    "'[[unlikely]]' can only be applied to if statements and case/default labels");
            }
            
            if (!attr.argument.empty()) {
                error(line, col, "'[[unlikely]]' attribute takes no arguments");
            }
        }
    }
}

void SemanticAnalyzer::visitStatement(Statement* stmt) {
    if (!stmt) return;
    
    if (auto* block = dynamic_cast<BlockStatement*>(stmt)) {
        Scope block_scope(current_scope, "block");
        Scope* old_scope = current_scope;
        current_scope = &block_scope;
        
        for (auto& s : block->statements) {
            visitStatement(s.get());
        }

        for (const auto& [name, sym] : block_scope.getSymbols()) {
            if (sym->kind == SymbolKind::Variable && !sym->is_used) {
                // Check if it has [[maybe_unused]]
                if (!sym->hasAttribute("maybe_unused")) {
                    warning(sym->line, sym->column,
                            "Unused variable '" + name + "'");
                }
            }
        }
        
        current_scope = old_scope;
    }
    else if (auto* var = dynamic_cast<VarDeclaration*>(stmt)) {
        validateAttributes(
            var->attributes,
            var->line,
            var->column,
            "variable '" + var->name + "'"
        );

        auto symbol = std::make_unique<Symbol>();
        symbol->name = var->name;
        symbol->kind = SymbolKind::Variable;
        symbol->type = cloneType(var->type.get());  // Need to implement this
        symbol->line = var->line;
        symbol->column = var->column;
        symbol->is_initialized = (var->initializer != nullptr);
        symbol->is_constexpr = var->is_constexpr;
        symbol->attributes = var->attributes;
        if (var->initializer) {
            visitExpr(var->initializer.get());
            
            const Type* var_type = var->type.get();
            const Type* init_type = var->initializer->resolved_type.get();
            
            if (!typesCompatible(init_type, var_type)) {
                error(var->line, var->column,
                    "Incompatible types in initialization of '" + var->name + "'");
            }
        }
        if (!current_scope->define(std::move(symbol))) {
            error(var->line, var->column, 
                  "Redefinition of '" + var->name + "'");
        }
        
        // Visit the initializer expression
        if (var->initializer) {
            visitExpr(var->initializer.get());
        }
    }
    else if (auto* func = dynamic_cast<FuncDeclaration*>(stmt)) {
        validateAttributes(func->attributes,
            func->line, func->column, 
            "function '" + func->name + "'"
        );
        for (const auto& attr : func->attributes) {
            if (attr.name == "noreturn") {
                if (auto* basic = dynamic_cast<const BasicType*>(func->return_type.get())) {
                    if (basic->kind != BasicType::Kind::Void) {
                        error(func->line, func->column,
                            "[[noreturn]] function '" + func->name + 
                            "' must have void return type");
                    }
                }
            }
        }
        auto symbol = std::make_unique<Symbol>();
        symbol->name = func->name;
        symbol->kind = SymbolKind::Function;
        symbol->type = cloneType(func->return_type.get());
        symbol->line = func->line;
        symbol->column = func->column;
        symbol->is_defined = (func->body != nullptr);
        
        for (auto& param : func->parameters) {
            symbol->param_types.push_back(cloneType(param.type.get()));
        }
        
        Symbol* existing = current_scope->resolve(func->name);
        if (existing) {
            if (existing->kind != SymbolKind::Function) {
                error(func->line, func->column,
                      "'" + func->name + "' redeclared as different kind of symbol");
            } else if (func->body && existing->is_defined) {
                error(func->line, func->column,
                      "Redefinition of function '" + func->name + "'");
            } else {
                // Update the existing symbol
                existing->is_defined = existing->is_defined || (func->body != nullptr);
            }
        } else {
            current_scope->define(std::move(symbol));
        }
        
        if (func->body) {
            const FuncDeclaration* old_func = current_function;
            current_function = func;  // Set context
            bool old_has_return = has_return;
            
            Scope func_scope(current_scope, "function:" + func->name);
            Scope* old_scope = current_scope;
            current_scope = &func_scope;
            
            for (auto& param : func->parameters) {
                auto param_sym = std::make_unique<Symbol>();
                param_sym->name = param.name;
                param_sym->kind = SymbolKind::Parameter;
                param_sym->type = cloneType(param.type.get());
                param_sym->line = func->line;
                param_sym->column = func->column;
                
                if (!current_scope->define(std::move(param_sym))) {
                    error(func->line, func->column,
                          "Duplicate parameter name '" + param.name + "'");
                }
            }
            
            visitStatement(func->body.get());

            if (!has_return) {
                if (auto* basic = dynamic_cast<const BasicType*>(func->return_type.get())) {
                    if (basic->kind != BasicType::Kind::Void && func->name != "main") {
                        Symbol* func_sym = current_scope->resolve(func->name);
                        if (!func_sym || !func_sym->hasAttribute("noreturn")) {
                            error(func->line, func->column,
                                "Non-void function '" + func->name + 
                                "' should return a value"
                            );
                        }
                    }
                }
            }
            current_scope = old_scope;
            current_function = old_func;
            has_return = old_has_return;
        }
    }
    else if (auto* expr_stmt = dynamic_cast<ExpressionStatement*>(stmt)) {
        if (expr_stmt->expr) {
            visitExpr(expr_stmt->expr.get());

            if (auto* call = dynamic_cast<CallExpr*>(expr_stmt->expr.get())) {
                if (auto* ident = dynamic_cast<IdentifierExpr*>(call->callee.get())) {
                    Symbol* sym = current_scope->resolve(ident->name);
                    if (sym && sym->hasAttribute("nodiscard")) {
                        std::string msg = "Ignoring return value of function '" + 
                                        ident->name + "' declared with [[nodiscard]]";
                        std::string reason = sym->getAttributeArg("nodiscard");
                        if (!reason.empty()) {
                            msg += ": " + reason;
                        }
                        error(call->line, call->column, msg);
                    }
                }
            }
        }
    }
    else if (auto* ret = dynamic_cast<ReturnStatement*>(stmt)) {
        if (!current_function) {
            error(ret->line, ret->column,
                "'return' outside of function");
            return;
        }
        Symbol* func_sym = current_scope->resolve(current_function->name);
        if (func_sym && func_sym->hasAttribute("noreturn")) {
            error(ret->line, ret->column,
                "Function '" + current_function->name + 
                "' declared [[noreturn]] should not return");
        }
        has_return = true;
        if (ret->expr) {
            visitExpr(ret->expr.get());
            
            const Type* func_ret_type = current_function->return_type.get();
            const Type* expr_type = ret->expr->resolved_type.get();
            
            if (auto* basic = dynamic_cast<const BasicType*>(func_ret_type)) {
                if (basic->kind == BasicType::Kind::Void) {
                    error(ret->line, ret->column,
                        "void function should not return a value");
                } else if (!typesCompatible(expr_type, func_ret_type)) {
                    error(ret->line, ret->column,
                        "Incompatible types in return statement");
                }
            }
        } else {
            if (current_function->name != "main") {
                if (auto* basic 
                        = dynamic_cast<const BasicType*>(current_function->return_type.get())) {
                    if (basic->kind != BasicType::Kind::Void) {
                        error(ret->line, ret->column,
                            "Non-void function '" + current_function->name + 
                            "' should return a value");
                    }
                }
            }
        }
    }
    else if (auto* if_stmt = dynamic_cast<IfStatement*>(stmt)) {
        validateAttributes(if_stmt->attributes, if_stmt->line, if_stmt->column, 
            "if statement");
        visitExpr(if_stmt->condition.get());
        visitStatement(if_stmt->thenBranch.get());
        if (if_stmt->elseBranch) {
            visitStatement(if_stmt->elseBranch.get());
        }
    }
    else if (auto* while_stmt = dynamic_cast<WhileStatement*>(stmt)) {
        visitExpr(while_stmt->condition.get());
        loop_depth++;
        visitStatement(while_stmt->body.get());
        loop_depth--;
    }
    else if (auto* for_stmt = dynamic_cast<ForStatement*>(stmt)) {
        Scope for_scope(current_scope, "for");
        Scope* old_scope = current_scope;
        current_scope = &for_scope;
        
        if (for_stmt->init) visitStatement(for_stmt->init.get());
        if (for_stmt->condition) visitExpr(for_stmt->condition.get());
        if (for_stmt->increment) visitExpr(for_stmt->increment.get());
        loop_depth++;
        visitStatement(for_stmt->body.get());
        loop_depth--;
        
        current_scope = old_scope;
    }
    else if (auto* attr_stmt = dynamic_cast<AttributeStatement*>(stmt)) {
        validateAttributes(attr_stmt->attributes, 
            stmt->line, stmt->column, "statement");
        
        for (const auto& attr : attr_stmt->attributes) {
            if (attr.name == "fallthrough" && switch_depth == 0) {
                error(stmt->line, stmt->column,
                    "'[[fallthrough]]' statement not within a switch");
            }
        }
    }

    else if (auto* switch_stmt = dynamic_cast<SwitchStatement*>(stmt)) {
        visitExpr(switch_stmt->condition.get());

        const Type* cond_type = switch_stmt->condition->resolved_type.get();
        if (!isNumericType(cond_type)) {
            error(switch_stmt->line, switch_stmt->column,
                "Switch condition must be an integer type");
        }
        
        switch_depth++;
        visitStatement(switch_stmt->body.get());
        switch_depth--;
    }
    else if (auto* case_stmt = dynamic_cast<CaseStatement*>(stmt)) {
        validateAttributes(case_stmt->attributes, case_stmt->line, case_stmt->column, 
            "case label");
        visitExpr(case_stmt->value.get());

        const Type* val_type = case_stmt->value->resolved_type.get();
        if (!isNumericType(val_type)) {
            error(case_stmt->line, case_stmt->column,
                "Case value must be an integer constant");
        }
        
        if (case_stmt->body) {
            visitStatement(case_stmt->body.get());
        }
    }
    else if (auto* default_stmt = dynamic_cast<DefaultStatement*>(stmt)) {
        validateAttributes(
            default_stmt->attributes,
            default_stmt->line,
            default_stmt->column, 
            "default label"
        );
        if (default_stmt->body) {
            visitStatement(default_stmt->body.get());
        }
    }
    else if (dynamic_cast<BreakStatement*>(stmt)) {
        if (loop_depth == 0 && switch_depth == 0) {
            error(stmt->line, stmt->column,
                "'break' statement not within loop or switch");
        }
    }
    else if (dynamic_cast<ContinueStatement*>(stmt)) {
        if (loop_depth == 0) {
            error(stmt->line, stmt->column,
                "'continue' statement not within a loop");
        }
    }
}

void SemanticAnalyzer::visitExpr(Expr* expr) {
    if (!expr) return;
    
    if (auto* num = dynamic_cast<NumberExpr*>(expr)) {
        auto type = std::make_unique<BasicType>();
        type->kind = num->is_float ? BasicType::Kind::Double : BasicType::Kind::Int;
        expr->resolved_type = std::move(type);
    }
    else if (auto* ident = dynamic_cast<IdentifierExpr*>(expr)) {
        Symbol* sym = current_scope->resolve(ident->name);
        if (!sym) {
            error(ident->line, ident->column,
                "Use of undeclared identifier '" + ident->name + "'");
        } else {
            sym->is_used = true;
            expr->resolved_type = cloneType(sym->type.get());
            
            if (sym->hasAttribute("deprecated")) {
                std::string msg = "'" + ident->name + "' is deprecated";
                std::string reason = sym->getAttributeArg("deprecated");
                if (!reason.empty()) {
                    msg += ": " + reason;
                }
                warning(ident->line, ident->column, msg);
            }
        }
    }
    else if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        visitExpr(bin->left.get());
        visitExpr(bin->right.get());
        
        const Type* left_type = bin->left->resolved_type.get();
        const Type* right_type = bin->right->resolved_type.get();

        TokenType op = bin->op;

        if (op == TokenType::Plus || op == TokenType::Minus ||
            op == TokenType::Star || op == TokenType::Slash ||
            op == TokenType::Percent) {
            
            if (!isNumericType(left_type) || !isNumericType(right_type)) {
                error(bin->line, bin->column,
                      "Invalid operands to binary expression");
            }

            if (op == TokenType::Plus || op == TokenType::Minus) {
                if (isPointerType(left_type) && isNumericType(right_type)) {
                    bin->resolved_type = cloneType(left_type);
                } else if (isNumericType(left_type) && isPointerType(right_type) && op == TokenType::Plus) {
                    bin->resolved_type = cloneType(right_type);
                } else if (isPointerType(left_type) && isPointerType(right_type) && op == TokenType::Minus) {
                    // ptr - ptr = integer (ptrdiff_t)
                    auto type = std::make_unique<BasicType>();
                    type->kind = BasicType::Kind::Long;
                    bin->resolved_type = std::move(type);
                } else {
                    bin->resolved_type = getCommonType(left_type, right_type);
                }
            } else {
                bin->resolved_type = getCommonType(left_type, right_type);
            }
        }
        else if (op == TokenType::Less || op == TokenType::Greater ||
                 op == TokenType::LessEqual || op == TokenType::GreaterEqual ||
                 op == TokenType::EqualEqual || op == TokenType::ExclaimEqual) {

            if (!isNumericType(left_type) && !isPointerType(left_type)) {
                error(bin->line, bin->column,
                      "Invalid left operand to comparison");
            }
            if (!isNumericType(right_type) && !isPointerType(right_type)) {
                error(bin->line, bin->column,
                      "Invalid right operand to comparison");
            }

            auto type = std::make_unique<BasicType>();
            type->kind = BasicType::Kind::Int;
            bin->resolved_type = std::move(type);
        }
        else if (op == TokenType::AmpAmp || op == TokenType::PipePipe) {
            // Operands can be any type (converted to bool)
            auto type = std::make_unique<BasicType>();
            type->kind = BasicType::Kind::Int;
            bin->resolved_type = std::move(type);
        }
        else if (op == TokenType::Ampersand || op == TokenType::Pipe ||
                 op == TokenType::Caret || op == TokenType::LShift ||
                 op == TokenType::RShift) {
            
            if (!isNumericType(left_type) || !isNumericType(right_type)) {
                error(bin->line, bin->column,
                      "Invalid operands to bitwise expression");
            }
            bin->resolved_type = getCommonType(left_type, right_type);
        }
        else if (op == TokenType::Equal || op == TokenType::PlusEqual ||
                 op == TokenType::MinusEqual || op == TokenType::StarEqual ||
                 op == TokenType::SlashEqual || op == TokenType::PercentEqual) {
            
            if (!isLvalue(bin->left.get())) {
                error(bin->line, bin->column,
                    "Expression is not assignable");
            }

            if (auto* ident = dynamic_cast<IdentifierExpr*>(bin->left.get())) {
                Symbol* sym = current_scope->resolve(ident->name);
                if (sym && sym->type) {
                    if (auto* basic 
                            = dynamic_cast<const BasicType*>(sym->type.get())) {
                        if (basic->is_const) {
                            error(bin->line, bin->column,
                                "Cannot assign to const variable '" +
                                ident->name + "'"
                            );
                        }
                    }
                }
            }

            if (!typesCompatible(right_type, left_type)) {
                error(bin->line, bin->column,
                      "Incompatible types in assignment");
            }
            bin->resolved_type = cloneType(left_type);
        }
    }
    else if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        visitExpr(unary->operand.get());
        
        const Type* operand_type = unary->operand->resolved_type.get();
        
        if (unary->op == TokenType::Minus || unary->op == TokenType::Plus) {
            if (!isNumericType(operand_type)) {
                error(unary->line, unary->column,
                      "Invalid operand to unary expression");
            }
            unary->resolved_type = cloneType(operand_type);
        }
        else if (unary->op == TokenType::Exclaim) {
            // Logical not - result is int (bool)
            auto type = std::make_unique<BasicType>();
            type->kind = BasicType::Kind::Int;
            unary->resolved_type = std::move(type);
        }
        else if (unary->op == TokenType::Tilde) {
            // Bitwise not - requires integer
            if (!isNumericType(operand_type)) {
                error(unary->line, unary->column,
                      "Invalid operand to bitwise not");
            }
            unary->resolved_type = cloneType(operand_type);
        }
        else if (unary->op == TokenType::Star) {
            // Dereference - operand must be pointer
            if (auto* ptr_type = dynamic_cast<const PointerType*>(operand_type)) {
                unary->resolved_type = cloneType(ptr_type->base_type.get());
            } else {
                error(unary->line, unary->column,
                      "Dereference of non-pointer type");
            }
        }
        else if (unary->op == TokenType::Ampersand) {
            // Address-of - result is pointer to operand type
            auto ptr_type = std::make_unique<PointerType>();
            ptr_type->base_type = cloneType(operand_type);
            unary->resolved_type = std::move(ptr_type);
        }
    }
    else if (auto* call = dynamic_cast<CallExpr*>(expr)) {
    visitExpr(call->callee.get());
    
    // Visit all arguments first
    for (auto& arg : call->arguments) {
        visitExpr(arg.get());
    }
    
    if (auto* ident = dynamic_cast<IdentifierExpr*>(call->callee.get())) {
        Symbol* sym = current_scope->resolve(ident->name);
        if (sym && sym->kind == SymbolKind::Function) {
            size_t expected = sym->param_types.size();
            size_t actual = call->arguments.size();
            
            if (actual < expected) {
                error(call->line, call->column,
                    "Too few arguments to function '" + ident->name +
                    "', expected " + std::to_string(expected) + 
                    " but got " + std::to_string(actual)
                );
            } else if (actual > expected) {
                error(call->line, call->column,
                    "Too many arguments to function '" + ident->name + 
                    "', expected " + std::to_string(expected) + 
                    " but got " + std::to_string(actual)
                );
            }
            
            for (size_t i = 0; i < std::min(actual, expected); i++) {
                const Type* arg_type = call->arguments[i]->resolved_type.get();
                const Type* param_type = sym->param_types[i].get();
                
                if (!typesCompatible(arg_type, param_type)) {
                    error(call->arguments[i]->line, call->arguments[i]->column,
                        "Incompatible type for argument " + std::to_string(i + 1) + 
                        " of function '" + ident->name + "'"
                    );
                }
            }
            
            call->resolved_type = cloneType(sym->type.get());
        } else if (sym && sym->kind != SymbolKind::Function) {
            error(call->line, call->column,
                "Called object '" + ident->name + "' is not a function"
            );
        }
    }
}
    else if (auto* index = dynamic_cast<IndexExpr*>(expr)) {
        visitExpr(index->array.get());
        visitExpr(index->index.get());
        
        const Type* array_type = index->array->resolved_type.get();
        const Type* index_type = index->index->resolved_type.get();
        
        // Array must be array or pointer
        if (auto* arr_type = dynamic_cast<const ArrayType*>(array_type)) {
            index->resolved_type = cloneType(arr_type->element_type.get());
        } else if (auto* ptr_type = dynamic_cast<const PointerType*>(array_type)) {
            index->resolved_type = cloneType(ptr_type->base_type.get());
        } else {
            error(index->line, index->column,
                  "Subscripted value is not an array or pointer");
        }
        
        // Index must be integer
        if (!isNumericType(index_type)) {
            error(index->line, index->column,
                  "Array subscript is not an integer");
        }
    }
    else if (auto* cond = dynamic_cast<ConditionalExpr*>(expr)) {
        visitExpr(cond->condition.get());
        visitExpr(cond->trueExpr.get());
        visitExpr(cond->falseExpr.get());
        
        // Result type is common type of true and false branches
        cond->resolved_type = getCommonType(
            cond->trueExpr->resolved_type.get(),
            cond->falseExpr->resolved_type.get()
        );
    }
    else if (auto* postfix = dynamic_cast<PostfixExpr*>(expr)) {
        visitExpr(postfix->operand.get());
        
        const Type* operand_type = postfix->operand->resolved_type.get();
        if (!isNumericType(operand_type) && !isPointerType(operand_type)) {
            error(postfix->line, postfix->column,
                  "Invalid operand to increment/decrement");
        }
        postfix->resolved_type = cloneType(operand_type);
    }
    else if (auto* member = dynamic_cast<MemberExpr*>(expr)) {
        visitExpr(member->object.get());
        // TODO: Check struct/union type and member existence
    }
    // ParenExpr just inherits the inner type
    else if (auto* paren = dynamic_cast<ParenExpr*>(expr)) {
        visitExpr(paren->inner.get());
        paren->resolved_type = cloneType(paren->inner->resolved_type.get());
    }

    else if (auto* str = dynamic_cast<StringExpr*>(expr)) {
        auto ptr_type = std::make_unique<PointerType>();
        auto char_type = std::make_unique<BasicType>();
        char_type->kind = BasicType::Kind::Char;
        char_type->is_const = true;
        ptr_type->base_type = std::move(char_type);
        expr->resolved_type = std::move(ptr_type);
    }
    else if (auto* chr = dynamic_cast<CharExpr*>(expr)) {
        auto type = std::make_unique<BasicType>();
        type->kind = BasicType::Kind::Int;
        expr->resolved_type = std::move(type);
    }
}

bool SemanticAnalyzer::isLvalue(Expr* expr) {
    if (!expr) return false;
    if (dynamic_cast<IdentifierExpr*>(expr)) {
        return true;
    }
    if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        if (unary->op == TokenType::Star) {
            return true;
        }
    }

    if (dynamic_cast<IndexExpr*>(expr)) {
        return true;
    }

    if (dynamic_cast<MemberExpr*>(expr)) {
        return true;
    }

    if (auto* paren = dynamic_cast<ParenExpr*>(expr)) {
        return isLvalue(paren->inner.get());
    }

    return false;
}

std::string SemanticAnalyzer::typeToString(const Type* type) {
    if (!type) return "unknown";
    
    if (auto* basic = dynamic_cast<const BasicType*>(type)) {
        std::string result;
        if (basic->is_const) result += "const ";
        if (basic->is_unsigned) result += "unsigned ";
        
        switch (basic->kind) {
            case BasicType::Kind::Void: result += "void"; break;
            case BasicType::Kind::Bool: result += "bool"; break;
            case BasicType::Kind::Char: result += "char"; break;
            case BasicType::Kind::Short: result += "short"; break;
            case BasicType::Kind::Int: result += "int"; break;
            case BasicType::Kind::Long: result += "long"; break;
            case BasicType::Kind::Float: result += "float"; break;
            case BasicType::Kind::Double: result += "double"; break;
        }
        return result;
    }
    
    if (auto* ptr = dynamic_cast<const PointerType*>(type)) {
        return typeToString(ptr->base_type.get()) + "*";
    }
    
    if (auto* arr = dynamic_cast<const ArrayType*>(type)) {
        return typeToString(arr->element_type.get()) + "[]";
    }
    
    return "unknown";
}
}   // namespace dix