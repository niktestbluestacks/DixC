#ifndef EXPR_HPP
#define EXPR_HPP

// dix
#include <Token.hpp>

// std
#include <memory>
#include <string>
#include <vector>

namespace dix {
struct Expr {
    virtual ~Expr() = default;
    int line;
    int column;
};

struct NumberExpr : Expr {
    std::string value;
    bool is_float;
};

struct IdentifierExpr : Expr {
    std::string name;
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    TokenType op;
};

struct UnaryExpr : Expr {
    std::unique_ptr<Expr> operand;
    TokenType op;
};

struct ParenExpr : Expr {
    std::unique_ptr<Expr> inner;
};

struct ConditionalExpr : Expr {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> trueExpr;
    std::unique_ptr<Expr> falseExpr;
};

struct CallExpr : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
};

struct MemberExpr : Expr {
    std::unique_ptr<Expr> object;
    std::string member;
    bool is_arrow;
};

struct PostfixExpr : Expr {
    std::unique_ptr<Expr> operand;
    TokenType op;
};
}   // namespace dix

#endif // EXPR_HPP