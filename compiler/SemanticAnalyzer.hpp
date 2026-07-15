#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

// dix
#include <Expr.hpp>
#include <Ast.hpp>
#include <Semantic.hpp>

// std
#include <vector>
#include <string>
#include <set>

namespace dix {
enum class DiagnosticKind {
    Error,
    Warning
};

struct Diagnostic {
    DiagnosticKind kind;
    int line;
    int column;
    std::string message;
};

class SemanticAnalyzer {
private:
    std::vector<Diagnostic> diagnostics;
    Scope* current_scope;
    Scope global_scope{nullptr, "global"};

    const FuncDeclaration* current_function = nullptr;
    int loop_depth = 0;
    int switch_depth = 0;
    bool has_return = false;

    static const std::set<std::string> ValidAttributes;
    std::unordered_map<std::string, AttributeList> function_attributes;
    std::unordered_map<std::string, AttributeList> variable_attributes;

    void error(int line, int col, const std::string& msg) {
        diagnostics.push_back(
            {DiagnosticKind::Error, line, col, msg}
        );
    }
    
    void warning(int line, int col, const std::string& msg) {
        diagnostics.push_back(
            {DiagnosticKind::Warning, line, col, msg}
        );
    }
    void validateAttributes(
        const AttributeList& attrs,
        int line, int col,
        const std::string& context
    );


    void visitStatement(Statement* stmt);
    void visitExpr(Expr* expr);
    void visitDeclaration(Statement* stmt);
    bool isLvalue(Expr* expr);
    std::string typeToString(const Type* type);
public:
    std::vector<Diagnostic> analyze(Statement* program);
};
}   // namespace dix
#endif // SEMANTIC_ANALYZER_HPP