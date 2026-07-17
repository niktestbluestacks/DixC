#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

// dix
#include <Ast.hpp>
#include <Expr.hpp>
#include <Semantic.hpp>

// std
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace dix {
enum class DiagnosticKind { Error, Warning };

struct Diagnostic {
  DiagnosticKind kind;
  int line;
  int column;
  std::string message;
};

class SemanticAnalyzer {
 private:
  std::vector<Diagnostic> diagnostics;

  // scope manadgement
  std::vector<std::unique_ptr<Scope>> owned_scopes;
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
    diagnostics.push_back({DiagnosticKind::Error, line, col, msg});
  }

  void warning(int line, int col, const std::string& msg) {
    diagnostics.push_back({DiagnosticKind::Warning, line, col, msg});
  }
  void validateAttributes(const AttributeList& attrs, int line, int col,
                          const std::string& context);

  void enterScope(const std::string& name);
  void exitScope();

  bool isLvalue(Expr* expr);
  std::string typeToString(const Type* type);

 public:
  SemanticAnalyzer() : current_scope(&global_scope) {}
  std::vector<Diagnostic> analyze(Statement* program);

  // Statements
  void visit(BlockStatement* stmt);
  void visit(VarDeclaration* stmt);
  void visit(FuncDeclaration* stmt);
  void visit(ExpressionStatement* stmt);
  void visit(ReturnStatement* stmt);
  void visit(IfStatement* stmt);
  void visit(ElseStatement* stmt);
  void visit(WhileStatement* stmt);
  void visit(ForStatement* stmt);
  void visit(BreakStatement* stmt);
  void visit(ContinueStatement* stmt);
  void visit(SwitchStatement* stmt);
  void visit(CaseStatement* stmt);
  void visit(DefaultStatement* stmt);
  void visit(AttributeStatement* stmt);
  void visit(StructDecl* stmt);
  void visit(EnumDecl* stmt);
  void visit(TypedefDecl* stmt);
  // Expressions
  void visit(NumberExpr* expr);
  void visit(IdentifierExpr* expr);
  void visit(BinaryExpr* expr);
  void visit(UnaryExpr* expr);
  void visit(ParenExpr* expr);
  void visit(ConditionalExpr* expr);
  void visit(CallExpr* expr);
  void visit(StringExpr* expr);
  void visit(CharExpr* expr);
  void visit(IndexExpr* expr);
  void visit(MemberExpr* expr);
  void visit(PostfixExpr* expr);
};
}  // namespace dix
#endif  // SEMANTIC_ANALYZER_HPP