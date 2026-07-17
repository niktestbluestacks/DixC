#include <Ast.hpp>
#include <SemanticAnalyzer.hpp>

namespace dix {
void ExpressionStatement::accept(SemanticAnalyzer& visitor) {
  visitor.visit(this);
}

void BlockStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void ReturnStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void WhileStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void ForStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void BreakStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void ContinueStatement::accept(SemanticAnalyzer& visitor) {
  visitor.visit(this);
}

void SwitchStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void ElseStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void IfStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void CaseStatement::accept(SemanticAnalyzer& visitor) { visitor.visit(this); }

void DefaultStatement::accept(SemanticAnalyzer& visitor) {
  visitor.visit(this);
}

void AttributeStatement::accept(SemanticAnalyzer& visitor) {
  visitor.visit(this);
}

void FuncDeclaration::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void EnumDecl::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void TypedefDecl::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void StructDecl::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}

void VarDeclaration::accept(SemanticAnalyzer& visitor) {
    visitor.visit(this);
}
}  // namespace dix