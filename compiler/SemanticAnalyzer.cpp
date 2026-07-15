// dix
#include <SemanticAnalyzer.hpp>

namespace dix {
std::vector<SemanticError> SemanticAnalyzer::analyze(Statement* program) {
    errors.clear();
    current_scope = &global_scope;
    
    visitStatement(program);
    
    return errors;
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
        
        current_scope = old_scope;
    }
    else if (auto* var = dynamic_cast<VarDeclaration*>(stmt)) {
        auto symbol = std::make_unique<Symbol>();
        symbol->name = var->name;
        symbol->kind = SymbolKind::Variable;
        symbol->type = cloneType(var->type.get());  // Need to implement this
        symbol->line = var->line;
        symbol->column = var->column;
        symbol->is_initialized = (var->initializer != nullptr);
        symbol->is_constexpr = var->is_constexpr;
        
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
            current_scope = old_scope;
        }
    }
    else if (auto* expr_stmt = dynamic_cast<ExpressionStatement*>(stmt)) {
        if (expr_stmt->expr) {
            visitExpr(expr_stmt->expr.get());
        }
    }
    else if (auto* ret = dynamic_cast<ReturnStatement*>(stmt)) {
        if (ret->expr) {
            visitExpr(ret->expr.get());
        }
    }
    else if (auto* if_stmt = dynamic_cast<IfStatement*>(stmt)) {
        visitExpr(if_stmt->condition.get());
        visitStatement(if_stmt->thenBranch.get());
        if (if_stmt->elseBranch) {
            visitStatement(if_stmt->elseBranch.get());
        }
    }
    else if (auto* while_stmt = dynamic_cast<WhileStatement*>(stmt)) {
        visitExpr(while_stmt->condition.get());
        visitStatement(while_stmt->body.get());
    }
    else if (auto* for_stmt = dynamic_cast<ForStatement*>(stmt)) {
        Scope for_scope(current_scope, "for");
        Scope* old_scope = current_scope;
        current_scope = &for_scope;
        
        if (for_stmt->init) visitStatement(for_stmt->init.get());
        if (for_stmt->condition) visitExpr(for_stmt->condition.get());
        if (for_stmt->increment) visitExpr(for_stmt->increment.get());
        visitStatement(for_stmt->body.get());
        
        current_scope = old_scope;
    }
}

void SemanticAnalyzer::visitExpr(Expr* expr) {
    if (!expr) return;
    
    if (auto* ident = dynamic_cast<IdentifierExpr*>(expr)) {
        Symbol* sym = current_scope->resolve(ident->name);
        if (!sym) {
            error(ident->line, ident->column,
                  "Use of undeclared identifier '" + ident->name + "'");
        }
    }
    else if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        visitExpr(bin->left.get());
        visitExpr(bin->right.get());
        // TODO: Type check the operation
    }
    else if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        visitExpr(unary->operand.get());
    }
    else if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        visitExpr(call->callee.get());
        for (auto& arg : call->arguments) {
            visitExpr(arg.get());
        }
        // TODO: Check argument count/types match function signature
    }
    else if (auto* index = dynamic_cast<IndexExpr*>(expr)) {
        visitExpr(index->array.get());
        visitExpr(index->index.get());
    }
    else if (auto* member = dynamic_cast<MemberExpr*>(expr)) {
        visitExpr(member->object.get());
        // TODO: Check that object is a struct/union with this member
    }
    else if (auto* cond = dynamic_cast<ConditionalExpr*>(expr)) {
        visitExpr(cond->condition.get());
        visitExpr(cond->trueExpr.get());
        visitExpr(cond->falseExpr.get());
    }
    else if (auto* postfix = dynamic_cast<PostfixExpr*>(expr)) {
        visitExpr(postfix->operand.get());
    }
}
}   // namespace dix