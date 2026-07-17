#include <Expr.hpp>
#include <SemanticAnalyzer.hpp>

namespace dix {

void NumberExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void IdentifierExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void BinaryExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void UnaryExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void ParenExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void ConditionalExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void CallExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void StringExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void CharExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void IndexExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void MemberExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void PostfixExpr::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}
}   // namespace dix