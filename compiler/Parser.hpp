#ifndef PARSER_HPP
#define PARSER_HPP

// dix
#include <TokenStream.hpp>
#include <Expr.hpp>
#include <Ast.hpp>

// std
#include <memory>
#include <string_view>

namespace dix {
class Parser {
private:
    TokenStream stream;

    // Expressions
    std::unique_ptr<Expr> parseAssignment(); // = += -= *= /= %= &= |= ^= <<= >>=
    std::unique_ptr<Expr> parseTernary(); // ?:
    std::unique_ptr<Expr> parseLogicalOr(); // ||
    std::unique_ptr<Expr> parseLogicalAnd(); // &&
    std::unique_ptr<Expr> parseBitwiseOr(); // |
    std::unique_ptr<Expr> parseBitwiseXor(); // ^
    std::unique_ptr<Expr> parseBitwiseAnd(); // &
    std::unique_ptr<Expr> parseEquality(); // != ==
    std::unique_ptr<Expr> parseComparison(); // > >= <= <
    std::unique_ptr<Expr> parseShift(); // << >>
    std::unique_ptr<Expr> parseAddition(); // + -
    std::unique_ptr<Expr> parseMultiplication(); // * / % 
    std::unique_ptr<Expr> parseUnary(); // + - ! ~
    std::unique_ptr<Expr> parsePostfix(); // postfix -- ++
    std::unique_ptr<Expr> parsePrimary(); // literals, identifiers, parenthesized expr

    // Statements
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseExpressionStatement();
    std::unique_ptr<Statement> parseBlockStatement();
    std::unique_ptr<Statement> parseReturnStatement();
    std::unique_ptr<Statement> parseIfStatement();
    std::unique_ptr<Statement> parseElseStatement();
    std::unique_ptr<Statement> parseWhileStatement();
    std::unique_ptr<Statement> parseForStatement();
    std::unique_ptr<Statement> parseSwitchStatement();
    std::unique_ptr<Statement> parseCaseOrDefaultLabel();

    // Declarations
    std::unique_ptr<Type> parseTypeSpecifier();
    Declarator parseDeclarator(std::unique_ptr<Type> base_type);
    std::unique_ptr<Statement> parseDeclaration();
    Parameter parseParameter();

    // Attributes
    AttributeList parseAttributes();

    // Helpers
    bool isTypeSpecifier();
public:
    Parser(std::string_view source): stream{source} {}
    std::unique_ptr<Expr> parseExpression() {
        return parseAssignment();
    }

    std::unique_ptr<Statement> parseProgram();
};
}   // namespace dix

#endif // PARSER_HPP