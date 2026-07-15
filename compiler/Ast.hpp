#ifndef AST_HPP
#define AST_HPP

// dix
#include <Expr.hpp>

// std
#include <vector>

namespace dix {

struct Statement {
    virtual ~Statement() = default;
    int line;
    int column;
};

struct ExpressionStatement : Statement {
    std::unique_ptr<Expr> expr;
};

struct BlockStatement : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
};

struct ReturnStatement : Statement {
    std::unique_ptr<Expr> expr;
};

struct IfStatement : Statement {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
};

struct WhileStatement : Statement {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> body;
};

struct ForStatement : Statement {
    std::unique_ptr<Statement> init;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Statement> body;
};

struct BreakStatement : Statement {};

struct ContinueStatement : Statement {};

struct Attribute {
    std::string name;
    std::vector<std::string> arguments;
};

using AttributeList = std::vector<Attribute>;

struct AttributeStatement : Statement {
    AttributeList attributes;
};

struct Type {
    virtual ~Type() = default;
    int line;
    int column;
};

struct BasicType : Type {
    enum class Kind { Void, Bool, Char, Short, Int, Long, Float, Double };
    Kind kind;
    bool is_unsigned = false;
    bool is_const = false;
    bool is_volatile = false;
    bool is_restrict = false;
};

struct PointerType : Type {
    std::unique_ptr<Type> base_type;
    bool is_const = false;
    bool is_volatile = false;
    bool is_restrict = false;
};

struct ArrayType : Type {
    std::unique_ptr<Type> element_type;
    std::unique_ptr<Expr> size;
};

struct FunctionType : Type {
    std::unique_ptr<Type> return_type;
    std::vector<std::unique_ptr<Type>> param_types;
    std::vector<std::string> param_names;
    bool is_variadic = false;
};

struct Declarator {
    std::string name;
    std::unique_ptr<Type> type;
};

struct VarDeclaration : Statement {
    std::unique_ptr<Type> type;
    std::string name;
    std::unique_ptr<Expr> initializer;
    bool is_constexpr = false;
};

struct Parameter {
    std::unique_ptr<Type> type;
    std::string name;
};

struct FuncDeclaration : Statement {
    std::unique_ptr<Type> return_type;
    std::string name;
    std::vector<Parameter> parameters;
    bool is_variadic = false;
    std::unique_ptr<Statement> body;
    AttributeList attributes;
};
}   // namespace dix

#endif // AST_HPP