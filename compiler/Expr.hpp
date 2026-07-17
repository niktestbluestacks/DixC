#ifndef EXPR_HPP
#define EXPR_HPP

// dix
#include <Token.hpp>

// std
#include <memory>
#include <string>
#include <vector>

namespace dix {
struct Type;
class SemanticAnalyzer;
struct Expr {
  virtual ~Expr() = default;
  int line;
  int column;
  std::unique_ptr<Type> resolved_type;
  virtual void accept(SemanticAnalyzer& visitor) = 0;
};

struct NumberExpr final : Expr {
  std::string value;
  bool is_float;
  void accept(SemanticAnalyzer& visitor) override;
};

struct IdentifierExpr final : Expr {
  std::string name;
  void accept(SemanticAnalyzer& visitor) override;
};

struct BinaryExpr final : Expr {
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;
  TokenType op;
  void accept(SemanticAnalyzer& visitor) override;
};

struct UnaryExpr final : Expr {
  std::unique_ptr<Expr> operand;
  TokenType op;
  void accept(SemanticAnalyzer& visitor) override;
};

struct ParenExpr final : Expr {
  std::unique_ptr<Expr> inner;
  void accept(SemanticAnalyzer& visitor) override;
};

struct ConditionalExpr final : Expr {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> trueExpr;
  std::unique_ptr<Expr> falseExpr;
  void accept(SemanticAnalyzer& visitor) override;
};

struct CallExpr final : Expr {
  std::unique_ptr<Expr> callee;
  std::vector<std::unique_ptr<Expr>> arguments;
  void accept(SemanticAnalyzer& visitor) override;
};

struct StringExpr final : Expr {
  std::string value;  // with quotes
  void accept(SemanticAnalyzer& visitor) override;
};

struct CharExpr final : Expr {
  std::string value;  // with quotes
  void accept(SemanticAnalyzer& visitor) override;
};

struct IndexExpr final : Expr {
  std::unique_ptr<Expr> array;
  std::unique_ptr<Expr> index;
  void accept(SemanticAnalyzer& visitor) override;
};

struct MemberExpr final : Expr {
  std::unique_ptr<Expr> object;
  std::string member;
  bool is_arrow;
  void accept(SemanticAnalyzer& visitor) override;
};

struct PostfixExpr final : Expr {
  std::unique_ptr<Expr> operand;
  TokenType op;
  void accept(SemanticAnalyzer& visitor) override;
};
}  // namespace dix

#endif  // EXPR_HPP