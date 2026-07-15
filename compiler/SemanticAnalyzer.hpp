#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

// dix
#include "Expr.hpp"
#include <Ast.hpp>
#include <Semantic.hpp>

// std
#include <vector>
#include <string>

namespace dix {
struct SemanticError {
    int line;
    int column;
    std::string message;
};

class SemanticAnalyzer {
private:
    std::vector<SemanticError> errors;
    Scope* current_scope;
    Scope global_scope{nullptr, "global"};

    void error(int line, int col, const std::string& msg) {
        errors.push_back({line, col, msg});
    }

    void visitStatement(Statement* stmt);
    void visitExpr(Expr* expr);
    void visitDeclaration(Statement* stmt);
public:
    std::vector<SemanticError> analyze(Statement* program);
};
}   // namespace dix
#endif // SEMANTIC_ANALYZER_HPP