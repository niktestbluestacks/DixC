#ifndef AST_HPP
#define AST_HPP

// dix
#include <Expr.hpp>

// std
#include <vector>
#include <memory>

namespace dix {
class SemanticAnalyzer;
struct Statement {
    virtual ~Statement() = default;
    int line;
    int column;
    virtual void accept(SemanticAnalyzer& visitor) = 0;
};

struct ExpressionStatement final :Statement {
    std::unique_ptr<Expr> expr;
    void accept(SemanticAnalyzer& visitor) override;
};

struct BlockStatement final :Statement {
    std::vector<std::unique_ptr<Statement>> statements;
    void accept(SemanticAnalyzer& visitor) override;
};

struct ReturnStatement final :Statement {
    std::unique_ptr<Expr> expr;
    void accept(SemanticAnalyzer& visitor) override;
};

struct WhileStatement final :Statement {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> body;
    void accept(SemanticAnalyzer& visitor) override;
};

struct ForStatement final :Statement {
    std::unique_ptr<Statement> init;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Statement> body;
    void accept(SemanticAnalyzer& visitor) override;
};

struct BreakStatement final :Statement {
    void accept(SemanticAnalyzer& visitor) override;
};

struct ContinueStatement final :Statement {
    void accept(SemanticAnalyzer& visitor) override;
};

struct SwitchStatement final :Statement {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> body;
    void accept(SemanticAnalyzer& visitor) override;
};

struct Attribute {
    std::string name;
    std::string argument;
};

using AttributeList = std::vector<Attribute>;

struct ElseStatement final :Statement {
    std::unique_ptr<Statement> body;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct IfStatement final :Statement {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<ElseStatement> elseBranch;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};


struct CaseStatement final :Statement {
    std::unique_ptr<Expr> value;
    std::unique_ptr<Statement> body;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct DefaultStatement final :Statement {
    std::unique_ptr<Statement> body;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct AttributeStatement final :Statement {
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct Type {
    virtual ~Type() = default;
    int line;
    int column;
};

struct BasicType final :Type {
    enum class Kind { Void, Bool, Char, Short, Int, Long, Float, Double };
    Kind kind;
    bool is_unsigned = false;
    bool is_const = false;
    bool is_volatile = false;
    bool is_restrict = false;
};

struct PointerType final :Type {
    std::unique_ptr<Type> base_type;
    bool is_const = false;
    bool is_volatile = false;
    bool is_restrict = false;
};

struct ArrayType final :Type {
    std::unique_ptr<Type> element_type;
    std::unique_ptr<Expr> size;
};

struct FunctionType final :Type {
    std::unique_ptr<Type> return_type;
    std::vector<std::unique_ptr<Type>> param_types;
    std::vector<std::string> param_names;
    bool is_variadic = false;
};

struct StructDecl;
struct UnionDecl;
struct EnumDecl;
struct StructMember;
struct EnumConstant;

struct StructType final :Type {
    std::string name;
    std::vector<StructMember> members;
    bool is_union = false;
    int size = 0;
    int alignment = 0;
};

struct EnumType final :Type {
    std::string name;
    std::vector<EnumConstant> constants;
    bool is_complete = false;
};

struct TypedefType final :Type {
    std::string name;
    std::unique_ptr<Type> underlying_type;
};

struct StructMember {
    std::string name;
    std::unique_ptr<Type> type;
    int bitfield_width = 0;
    int line;
    int column;
};

struct StructDecl final :Statement {
    std::string name;
    std::vector<StructMember> members;
    bool is_union = false;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct EnumConstant {
    std::string name;
    std::unique_ptr<Expr> value;
    int line;
    int column;
};

struct EnumDecl final :Statement {
    std::string name;
    std::vector<EnumConstant> constants;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct TypedefDecl final :Statement {
    std::string name;
    std::unique_ptr<Type> underlying_type;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct Declarator {
    std::string name;
    std::unique_ptr<Type> type;
};

struct VarDeclaration final :Statement {
    std::unique_ptr<Type> type;
    std::string name;
    std::unique_ptr<Expr> initializer;
    bool is_constexpr = false;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

struct Parameter {
    std::unique_ptr<Type> type;
    std::string name;
};

struct FuncDeclaration final :Statement {
    std::unique_ptr<Type> return_type;
    std::string name;
    std::vector<Parameter> parameters;
    bool is_variadic = false;
    std::unique_ptr<Statement> body;
    AttributeList attributes;
    void accept(SemanticAnalyzer& visitor) override;
};

inline std::unique_ptr<Type> cloneType(const Type* type) {
    if (!type) return nullptr;
    
    if (auto* basic = dynamic_cast<const BasicType*>(type)) {
        auto copy = std::make_unique<BasicType>();
        copy->kind = basic->kind;
        copy->is_unsigned = basic->is_unsigned;
        copy->is_const = basic->is_const;
        copy->is_volatile = basic->is_volatile;
        copy->is_restrict = basic->is_restrict;
        copy->line = basic->line;
        copy->column = basic->column;
        return copy;
    }
    else if (auto* ptr = dynamic_cast<const PointerType*>(type)) {
        auto copy = std::make_unique<PointerType>();
        copy->base_type = cloneType(ptr->base_type.get());
        copy->is_const = ptr->is_const;
        copy->is_volatile = ptr->is_volatile;
        copy->is_restrict = ptr->is_restrict;
        copy->line = ptr->line;
        copy->column = ptr->column;
        return copy;
    }
    else if (auto* arr = dynamic_cast<const ArrayType*>(type)) {
        auto copy = std::make_unique<ArrayType>();
        copy->element_type = cloneType(arr->element_type.get());
        if (arr->size) {
            // TODO: deep clone expression
        }
        copy->line = arr->line;
        copy->column = arr->column;
        return copy;
    }
    
    return nullptr;
}

inline bool typesCompatible(const Type* from, const Type* to) {
    if (!from || !to) return false;

    if (auto* from_basic = dynamic_cast<const BasicType*>(from)) {
        if (auto* to_basic = dynamic_cast<const BasicType*>(to)) {
            return from_basic->kind == to_basic->kind &&
                from_basic->is_unsigned == to_basic->is_unsigned;
        }
    }
    
    if (auto* from_ptr = dynamic_cast<const PointerType*>(from)) {
        if (auto* to_ptr = dynamic_cast<const PointerType*>(to)) {
            // void* can convert to any pointer
            if (auto* to_base = dynamic_cast<const BasicType*>(to_ptr->base_type.get())) {
                if (to_base->kind == BasicType::Kind::Void) return true;
            }
            if (auto* from_base = dynamic_cast<const BasicType*>(from_ptr->base_type.get())) {
                if (from_base->kind == BasicType::Kind::Void) return true;
            }
            return typesCompatible(from_ptr->base_type.get(), to_ptr->base_type.get());
        }
    }

    if (auto* from_arr = dynamic_cast<const ArrayType*>(from)) {
        if (auto* to_ptr = dynamic_cast<const PointerType*>(to)) {
            return typesCompatible(from_arr->element_type.get(), to_ptr->base_type.get());
        }
    }
    
    return false;
}

inline std::unique_ptr<Type> getCommonType(const Type* left, const Type* right) {
    if (!left || !right) return nullptr;
    
    auto* left_basic = dynamic_cast<const BasicType*>(left);
    auto* right_basic = dynamic_cast<const BasicType*>(right);
    
    if (left_basic && right_basic) {
        if (left_basic->kind == BasicType::Kind::Double || 
            right_basic->kind == BasicType::Kind::Double) {
            auto result = std::make_unique<BasicType>();
            result->kind = BasicType::Kind::Double;
            return result;
        }
        
        if (left_basic->kind == BasicType::Kind::Float || 
            right_basic->kind == BasicType::Kind::Float) {
            auto result = std::make_unique<BasicType>();
            result->kind = BasicType::Kind::Float;
            return result;
        }
        
        auto result = std::make_unique<BasicType>();
        result->kind = (left_basic->kind >= right_basic->kind) ? 
                       left_basic->kind : right_basic->kind;
        result->is_unsigned = left_basic->is_unsigned || right_basic->is_unsigned;
        return result;
    }
    
    if (dynamic_cast<const PointerType*>(left) && right_basic) {
        return cloneType(left);
    }
    if (left_basic && dynamic_cast<const PointerType*>(right)) {
        return cloneType(right);
    }
    
    return nullptr;
}

inline bool isNumericType(const Type* type) {
    return dynamic_cast<const BasicType*>(type) != nullptr;
}

inline bool isPointerType(const Type* type) {
    return dynamic_cast<const PointerType*>(type) != nullptr;
}
}   // namespace dix

#endif // AST_HPP