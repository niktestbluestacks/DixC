// dix
#include "Ast.hpp"
#include <Parser.hpp>

// std
#include <memory>

namespace dix {
std::unique_ptr<Statement> Parser::parseProgram() {
    auto program = std::make_unique<BlockStatement>();
    program->line = 1;
    program->column = 1;
    
    while (!stream.isAtEnd()) {
        AttributeList attrs = parseAttributes();
        
        if (stream.isAtEnd()) break;
        
        auto decl = parseDeclaration();
        
        if (!attrs.empty()) {
            if (auto* func = dynamic_cast<FuncDeclaration*>(decl.get())) {
                func->attributes = std::move(attrs);
            }
        }
        
        program->statements.push_back(std::move(decl));
    }
    
    return program;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    Token current = stream.current();
    
    if (current.type == TokenType::IntLiteral || 
        current.type == TokenType::FloatLiteral) {
        stream.advance();
        
        auto node = std::make_unique<NumberExpr>();
        node->value = std::string(current.text);
        node->is_float = (current.type == TokenType::FloatLiteral);
        node->line = current.line;
        node->column = current.column;
        return node;
    }

    if (current.type == TokenType::StringLiteral) {
        stream.advance();
        auto node = std::make_unique<StringExpr>();
        node->value = std::string(current.text);
        node->line = current.line;
        node->column = current.column;
        return node;
    }

    if (current.type == TokenType::CharLiteral) {
        stream.advance();
        auto node = std::make_unique<CharExpr>();
        node->value = std::string(current.text);
        node->line = current.line;
        node->column = current.column;
        return node;
    }
    
    if (current.type == TokenType::Identifier) {
        stream.advance();
        
        auto node = std::make_unique<IdentifierExpr>();
        node->name = std::string(current.text);
        node->line = current.line;
        node->column = current.column;
        return node;
    }
    
    if (current.type == TokenType::LParen) {
        stream.advance();
        
        auto inner = parseExpression();
        
        stream.expect(TokenType::RParen, "Expected ')' after expression");
        
        auto node = std::make_unique<ParenExpr>();
        node->inner = std::move(inner);
        node->line = current.line;
        node->column = current.column;
        return node;
    }
    
    throw std::runtime_error(
        "Expected expression at line " 
        + std::to_string(current.line)
        + ", column " + std::to_string(current.column)
    );
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    
    while (true) {
        Token current = stream.current();
        
        if (current.type == TokenType::LParen) {
            stream.advance();
            
            auto call = std::make_unique<CallExpr>();
            call->callee = std::move(expr);
            call->line = current.line;
            call->column = current.column;
            
            if (stream.current().type != TokenType::RParen) {
                call->arguments.push_back(parseAssignment());
                while (stream.current().type == TokenType::Comma) {
                    stream.advance();
                    call->arguments.push_back(parseAssignment());
                }
            }
            
            stream.expect(TokenType::RParen, "Expected ')' after arguments");
            expr = std::move(call);
        }
        else if (current.type == TokenType::LBracket) {
            stream.advance();
            
            auto index = std::make_unique<IndexExpr>();
            index->array = std::move(expr);
            index->index = parseExpression();
            index->line = current.line;
            index->column = current.column;
            
            stream.expect(TokenType::RBracket, "Expected ']' after index");
            expr = std::move(index);
        }
        else if (current.type == TokenType::Dot || current.type == TokenType::Arrow) {
            bool is_arrow = (current.type == TokenType::Arrow);
            stream.advance();
            
            if (stream.current().type != TokenType::Identifier) {
                throw std::runtime_error(
                    "Expected member name after " + 
                    std::string(is_arrow ? "->" : ".") +
                    " at line " + std::to_string(current.line)
                );
            }
            
            auto member = std::make_unique<MemberExpr>();
            member->object = std::move(expr);
            member->member = std::string(stream.current().text);
            member->is_arrow = is_arrow;
            member->line = current.line;
            member->column = current.column;
            
            stream.advance();
            expr = std::move(member);
        }
        else if (current.type == TokenType::PlusPlus || current.type == TokenType::MinusMinus) {
            stream.advance();
            
            auto postfix = std::make_unique<PostfixExpr>();
            postfix->operand = std::move(expr);
            postfix->op = current.type;
            postfix->line = current.line;
            postfix->column = current.column;
            
            expr = std::move(postfix);
        }
        else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    Token current = stream.current();

    if (current.type == TokenType::Minus ||
        current.type == TokenType::Exclaim ||
        current.type == TokenType::Tilde ||
        current.type == TokenType::Plus ||
        current.type == TokenType::Star ||
        current.type == TokenType::Ampersand) {
        
        stream.advance();
        
        auto operand = parseUnary();
        
        auto node = std::make_unique<UnaryExpr>();
        node->operand = std::move(operand);
        node->op = current.type;
        node->line = current.line;
        node->column = current.column;
        return node;
    }
    
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parseMultiplication() {
    auto left = parseUnary();
    
    while (stream.current().type == TokenType::Star ||
           stream.current().type == TokenType::Slash ||
           stream.current().type == TokenType::Percent) {
        
        TokenType op = stream.advance().type; 
        auto right = parseUnary(); 
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseAddition() {
    auto left = parseMultiplication();
    
    while (stream.current().type == TokenType::Plus ||
           stream.current().type == TokenType::Minus) {
        
        TokenType op = stream.advance().type;
        auto right = parseMultiplication();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseShift() {
    auto left = parseAddition();
    
    while (stream.current().type == TokenType::LShift ||
           stream.current().type == TokenType::RShift) {
        
        TokenType op = stream.advance().type;
        auto right = parseAddition();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto left = parseAddition();
    
    // While we see <, >, <=, or >=, keep parsing
    while (stream.current().type == TokenType::Less ||
           stream.current().type == TokenType::Greater ||
           stream.current().type == TokenType::LessEqual ||
           stream.current().type == TokenType::GreaterEqual) {
        
        TokenType op = stream.advance().type;
        auto right = parseAddition();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseComparison();
    
    while (stream.current().type == TokenType::EqualEqual ||
           stream.current().type == TokenType::ExclaimEqual) {
        
        TokenType op = stream.advance().type;
        auto right = parseComparison();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseAnd() {
    auto left = parseComparison();
    
    while (stream.current().type == TokenType::Ampersand) {
        
        TokenType op = stream.advance().type;
        auto right = parseComparison();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseXor() {
    auto left = parseBitwiseAnd();
    
    // While we see ^, keep parsing
    while (stream.current().type == TokenType::Caret) {
        
        TokenType op = stream.advance().type;
        auto right = parseBitwiseAnd();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseOr() {
    auto left = parseBitwiseXor();
    
    while (stream.current().type == TokenType::Pipe) {
        TokenType op = stream.advance().type;
        auto right = parseBitwiseXor();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseBitwiseOr();
    
    while (stream.current().type == TokenType::AmpAmp) {
        TokenType op = stream.advance().type;
        auto right = parseBitwiseOr();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    
    while (stream.current().type == TokenType::PipePipe) {
        TokenType op = stream.advance().type;
        auto right = parseLogicalAnd();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        left = std::move(node);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseTernary() {
    auto condition = parseLogicalOr();
    
    while (stream.current().type == TokenType::Question) {
        stream.advance();
        auto trueExpr = parseExpression(); 
        stream.expect(TokenType::Colon, 
            "Expected ':' after true branch of ternary expression"
        );
        auto falseExpr = parseTernary();

        auto node = std::make_unique<ConditionalExpr>();
        node->line = condition->line;
        node->column = condition->column;
        node->condition = std::move(condition);
        node->trueExpr = std::move(trueExpr);
        node->falseExpr = std::move(falseExpr);

        return node;
    }
    
    return condition;
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto left = parseTernary();
    
    if (stream.current().type == TokenType::Equal ||
        stream.current().type == TokenType::PlusEqual ||
        stream.current().type == TokenType::MinusEqual ||
        stream.current().type == TokenType::StarEqual ||
        stream.current().type == TokenType::SlashEqual ||
        stream.current().type == TokenType::PercentEqual ||
        stream.current().type == TokenType::AmpEqual ||
        stream.current().type == TokenType::PipeEqual ||
        stream.current().type == TokenType::CaretEqual ||
        stream.current().type == TokenType::LShiftEqual ||
        stream.current().type == TokenType::RShiftEqual) {
        
        TokenType op = stream.advance().type;
        
        auto right = parseAssignment();
        
        auto node = std::make_unique<BinaryExpr>();
        node->left = std::move(left);
        node->right = std::move(right);
        node->op = op;
        node->line = stream.current().line;
        node->column = stream.current().column;
        
        return node;
    }
    
    return left;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    Token current = stream.current();
    
    if (current.type == TokenType::KW_void ||
        current.type == TokenType::KW_bool ||
        current.type == TokenType::KW_char ||
        current.type == TokenType::KW_short ||
        current.type == TokenType::KW_int ||
        current.type == TokenType::KW_long ||
        current.type == TokenType::KW_float ||
        current.type == TokenType::KW_double ||
        current.type == TokenType::KW_signed ||
        current.type == TokenType::KW_unsigned ||
        current.type == TokenType::KW_const ||
        current.type == TokenType::KW_volatile ||
        current.type == TokenType::KW_constexpr) {
        return parseDeclaration();
    }
    
    if (current.type == TokenType::DobleLBracket) {
        AttributeList attrs = parseAttributes();
        
        auto stmt = std::make_unique<AttributeStatement>();
        stmt->attributes = std::move(attrs);
        return stmt;
    }

    if (current.type == TokenType::LBrace) {
        return parseBlockStatement();
    }
    
    if (current.type == TokenType::KW_return) {
        return parseReturnStatement();
    }
    
    if (current.type == TokenType::KW_if) {
        return parseIfStatement();
    }
    
    if (current.type == TokenType::KW_while) {
        return parseWhileStatement();
    }
    
    if (current.type == TokenType::KW_for) {
        return parseForStatement();
    }

    if (current.type == TokenType::KW_switch) {
        return parseSwitchStatement();
    }

    if (current.type == TokenType::KW_case || current.type == TokenType::KW_default) {
        return parseCaseOrDefaultLabel();
    }
    
    if (current.type == TokenType::KW_break) {
        stream.advance();
        stream.expect(TokenType::Semicolon, "Expected ';' after 'break'");
        auto stmt = std::make_unique<BreakStatement>();
        stmt->line = current.line;
        stmt->column = current.column;
        return stmt;
    }
    
    if (current.type == TokenType::KW_continue) {
        stream.advance();
        stream.expect(TokenType::Semicolon, "Expected ';' after 'continue'");
        auto stmt = std::make_unique<ContinueStatement>();
        stmt->line = current.line;
        stmt->column = current.column;
        return stmt;
    }
    
    if (current.type == TokenType::Semicolon) {
        stream.advance();
        auto stmt = std::make_unique<ExpressionStatement>();
        stmt->expr = nullptr;
        stmt->line = current.line;
        stmt->column = current.column;
        return stmt;
    }
    
    return parseExpressionStatement();
}

std::unique_ptr<Statement> Parser::parseExpressionStatement() {
    Token start = stream.current();
    
    auto expr = parseExpression();
    
    stream.expect(TokenType::Semicolon, "Expected ';' after expression");
    
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->expr = std::move(expr);
    stmt->line = start.line;
    stmt->column = start.column;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseBlockStatement() {
    Token start = stream.current();
    stream.expect(TokenType::LBrace, "Expected '{' to start block");
    
    auto stmt = std::make_unique<BlockStatement>();
    stmt->line = start.line;
    stmt->column = start.column;
    
    while (stream.current().type != TokenType::RBrace && !stream.isAtEnd()) {
        stmt->statements.push_back(parseStatement());
    }
    
    stream.expect(TokenType::RBrace, "Expected '}' to end block");
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseReturnStatement() {
    Token start = stream.current();
    stream.advance();
    
    auto stmt = std::make_unique<ReturnStatement>();
    stmt->line = start.line;
    stmt->column = start.column;
    
    if (stream.current().type != TokenType::Semicolon) {
        stmt->expr = parseExpression();
    }
    
    stream.expect(TokenType::Semicolon, "Expected ';' after return statement");
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseIfStatement() {
    Token start = stream.current();
    stream.advance();
    
    stream.expect(TokenType::LParen, "Expected '(' after 'if'");
    auto condition = parseExpression();
    stream.expect(TokenType::RParen, "Expected ')' after if condition");
    AttributeList attrs = parseAttributes();

    auto thenBranch = parseStatement();
    
    std::unique_ptr<ElseStatement> elseBranch = nullptr;
    if (stream.current().type == TokenType::KW_else) {
        Token elseStart = stream.current();
        stream.advance();
        auto body = parseStatement();

        elseBranch = std::make_unique<ElseStatement>();
        elseBranch->body = std::move(body);
        elseBranch->line = elseStart.line;
        elseBranch->column = elseStart.column;
    }
    
    auto stmt = std::make_unique<IfStatement>();
    stmt->condition = std::move(condition);
    stmt->thenBranch = std::move(thenBranch);
    stmt->elseBranch = std::move(elseBranch);
    stmt->attributes = std::move(attrs);
    stmt->line = start.line;
    stmt->column = start.column;
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseWhileStatement() {
    Token start = stream.current();
    stream.advance();
    
    stream.expect(TokenType::LParen, "Expected '(' after 'while'");
    auto condition = parseExpression();
    stream.expect(TokenType::RParen, "Expected ')' after while condition");
    
    auto body = parseStatement();
    
    auto stmt = std::make_unique<WhileStatement>();
    stmt->condition = std::move(condition);
    stmt->body = std::move(body);
    stmt->line = start.line;
    stmt->column = start.column;
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseForStatement() {
    Token start = stream.current();
    stream.advance();
    
    stream.expect(TokenType::LParen, "Expected '(' after 'for'");
    
    std::unique_ptr<Statement> init = nullptr;
    if (stream.current().type == TokenType::Semicolon) {
        stream.advance();
    } else if (isTypeSpecifier()) {
        init = parseDeclaration();
    } else {
        init = parseExpressionStatement();
    }
    
    std::unique_ptr<Expr> condition = nullptr;
    if (stream.current().type != TokenType::Semicolon) {
        condition = parseExpression();
    }
    stream.expect(TokenType::Semicolon, "Expected ';' after for condition");
    
    std::unique_ptr<Expr> increment = nullptr;
    if (stream.current().type != TokenType::RParen) {
        increment = parseExpression();
    }
    stream.expect(TokenType::RParen, "Expected ')' after for clauses");
    
    auto body = parseStatement();
    
    auto stmt = std::make_unique<ForStatement>();
    stmt->init = std::move(init);
    stmt->condition = std::move(condition);
    stmt->increment = std::move(increment);
    stmt->body = std::move(body);
    stmt->line = start.line;
    stmt->column = start.column;
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseSwitchStatement() {
    Token start = stream.current();
    stream.advance(); // consume 'switch'
    
    stream.expect(TokenType::LParen, "Expected '(' after 'switch'");
    auto condition = parseExpression();
    stream.expect(TokenType::RParen, "Expected ')' after switch condition");
    
    auto body = parseStatement();
    
    auto stmt = std::make_unique<SwitchStatement>();
    stmt->condition = std::move(condition);
    stmt->body = std::move(body);
    stmt->line = start.line;
    stmt->column = start.column;
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseCaseOrDefaultLabel() {
    Token start = stream.current();
    
    if (start.type == TokenType::KW_case) {
        stream.advance();
        
        AttributeList attrs = parseAttributes();

        auto value = parseExpression();
        stream.expect(TokenType::Colon, "Expected ':' after case value");
        
        std::unique_ptr<Statement> body = nullptr;
        if (stream.current().type != TokenType::KW_case &&
            stream.current().type != TokenType::KW_default &&
            stream.current().type != TokenType::RBrace) {
            body = parseStatement();
        }
        
        auto stmt = std::make_unique<CaseStatement>();
        stmt->value = std::move(value);
        stmt->body = std::move(body);
        stmt->attributes = std::move(attrs);
        stmt->line = start.line;
        stmt->column = start.column;
        
        return stmt;
    }
    else if (start.type == TokenType::KW_default) {
        stream.advance(); // consume 'default'
        
        AttributeList attrs = parseAttributes();
        
        stream.expect(TokenType::Colon, "Expected ':' after 'default'");
        
        std::unique_ptr<Statement> body = nullptr;
        if (stream.current().type != TokenType::KW_case &&
            stream.current().type != TokenType::KW_default &&
            stream.current().type != TokenType::RBrace) {
            body = parseStatement();
        }
        
        auto stmt = std::make_unique<DefaultStatement>();
        stmt->body = std::move(body);
        stmt->attributes = std::move(attrs);
        stmt->line = start.line;
        stmt->column = start.column;
        
        return stmt;
    }
    
    return nullptr;
}

std::unique_ptr<Type> Parser::parseTypeSpecifier() {
    Token start = stream.current();
    auto type = std::make_unique<BasicType>();
    type->line = start.line;
    type->column = start.column;
    
    while (stream.current().type == TokenType::KW_const ||
            stream.current().type == TokenType::KW_volatile ||
            stream.current().type == TokenType::KW_restrict) {
        if (stream.current().type == TokenType::KW_const) {
            type->is_const = true;
        } else if (stream.current().type == TokenType::KW_volatile) {
            type->is_volatile = true;
        } else {
            type->is_restrict = true;
        }
        stream.advance();
    }
    
    bool is_unsigned = false;
    if (stream.current().type == TokenType::KW_signed ||
        stream.current().type == TokenType::KW_unsigned) {
        is_unsigned = (stream.current().type == TokenType::KW_unsigned);
        stream.advance();
    }
    
    if (stream.current().type == TokenType::KW_void) {
        type->kind = BasicType::Kind::Void;
        stream.advance();
    } else if (stream.current().type == TokenType::KW_bool) {
        type->kind = BasicType::Kind::Bool;
        stream.advance();
    } else if (stream.current().type == TokenType::KW_char) {
        type->kind = BasicType::Kind::Char;
        type->is_unsigned = is_unsigned;
        stream.advance();
    } else if (stream.current().type == TokenType::KW_short) {
        type->kind = BasicType::Kind::Short;
        type->is_unsigned = is_unsigned;
        stream.advance();
    } else if (stream.current().type == TokenType::KW_int) {
        type->kind = BasicType::Kind::Int;
        type->is_unsigned = is_unsigned;
        stream.advance();
        
        if (stream.current().type == TokenType::KW_long) {
            stream.advance();
            if (stream.current().type == TokenType::KW_long) {
                type->kind = BasicType::Kind::Long;
                stream.advance();
            } else {
                type->kind = BasicType::Kind::Long;
            }
        }
    } else if (stream.current().type == TokenType::KW_long) {
        type->kind = BasicType::Kind::Long;
        type->is_unsigned = is_unsigned;
        stream.advance();
        
        if (stream.current().type == TokenType::KW_long) {
            stream.advance();
        }
        
        if (stream.current().type == TokenType::KW_int) {
            stream.advance();
        }
    } else if (stream.current().type == TokenType::KW_float) {
        type->kind = BasicType::Kind::Float;
        stream.advance();
    } else if (stream.current().type == TokenType::KW_double) {
        type->kind = BasicType::Kind::Double;
        stream.advance();
        
        if (stream.current().type == TokenType::KW_long) {
            stream.advance();
        }
    } else {
        throw std::runtime_error("Expected type specifier at line " + 
                                 std::to_string(stream.current().line));
    }
    
    while (stream.current().type == TokenType::KW_const ||
           stream.current().type == TokenType::KW_volatile ||
           stream.current().type == TokenType::KW_restrict) {
        if (stream.current().type == TokenType::KW_const) {
            type->is_const = true;
        } else if (stream.current().type == TokenType::KW_volatile) {
            type->is_volatile = true;
        } else if (stream.current().type == TokenType::KW_restrict) {
            type->is_restrict = true;
        }
        stream.advance();
    }
    
    return type;
}

Declarator Parser::parseDeclarator(std::unique_ptr<Type> base_type) {
    Declarator decl;
    
    while (stream.current().type == TokenType::Star) {
        stream.advance();
        
        auto ptr_type = std::make_unique<PointerType>();
        ptr_type->base_type = std::move(base_type);
        
                while (stream.current().type == TokenType::KW_const ||
               stream.current().type == TokenType::KW_volatile ||
               stream.current().type == TokenType::KW_restrict) {
            if (stream.current().type == TokenType::KW_const) {
                ptr_type->is_const = true;
            } else if (stream.current().type == TokenType::KW_volatile) {
                ptr_type->is_volatile = true;
            } else {
                ptr_type->is_restrict = true;
            }
            stream.advance();
        }
        
        base_type = std::move(ptr_type);
    }
    
    // Get the name
    if (stream.current().type == TokenType::Identifier) {
        decl.name = std::string(stream.current().text);
        stream.advance();
    } else {
        decl.name = "";
    }
    
    while (stream.current().type == TokenType::LBracket) {
        stream.advance();
        
        auto arr_type = std::make_unique<ArrayType>();
        arr_type->element_type = std::move(base_type);
        
        if (stream.current().type != TokenType::RBracket) {
            arr_type->size = parseExpression();
        }
        
        stream.expect(TokenType::RBracket, "Expected ']' after array size");
        
        base_type = std::move(arr_type);
    }
    
    decl.type = std::move(base_type);
    return decl;
}

Parameter Parser::parseParameter() {
    Parameter param;
    
    param.type = parseTypeSpecifier();
    
    Declarator decl = parseDeclarator(std::move(param.type));
    param.name = decl.name;
    param.type = std::move(decl.type);
    
    return param;
}

std::unique_ptr<Statement> Parser::parseDeclaration() {
    Token start = stream.current();
    
    AttributeList attrs = parseAttributes();

    bool is_constexpr = false;
    if (stream.current().type == TokenType::KW_constexpr) {
        is_constexpr = true;
        stream.advance();
    }
    
    auto base_type = parseTypeSpecifier();
    
    Declarator decl = parseDeclarator(std::move(base_type));
    
    if (stream.current().type == TokenType::LParen) {
        stream.advance();
        
        auto func = std::make_unique<FuncDeclaration>();
        func->return_type = std::move(decl.type);
        func->name = decl.name;
        func->line = start.line;
        func->column = start.column;
        
        if (stream.current().type != TokenType::RParen) {
            if (stream.current().type == TokenType::KW_void &&
                stream.peek().type == TokenType::RParen) {
                stream.advance();
            } else {
                func->parameters.push_back(parseParameter());
                
                while (stream.current().type == TokenType::Comma) {
                    stream.advance();
                    
                    if (stream.current().type == TokenType::Ellipsis) {
                        func->is_variadic = true;
                        stream.advance();
                        break;
                    }
                    
                    func->parameters.push_back(parseParameter());
                }
            }
        }
        
        stream.expect(TokenType::RParen, "Expected ')' after parameters");
        
        func->attributes = parseAttributes();
        
        if (stream.current().type == TokenType::LBrace) {
            func->body = parseBlockStatement();
        } else {
            stream.expect(TokenType::Semicolon, "Expected ';' or '{' after function declaration");
        }
        
        return func;
    }
    
    auto var = std::make_unique<VarDeclaration>();
    var->type = std::move(decl.type);
    var->name = decl.name;
    var->is_constexpr = is_constexpr;
    var->attributes = std::move(attrs);
    var->line = start.line;
    var->column = start.column;
    
    if (stream.current().type == TokenType::Equal) {
        stream.advance();
        var->initializer = parseExpression();
    }
    
    stream.expect(TokenType::Semicolon, "Expected ';' after variable declaration");
    
    return var;
}
AttributeList Parser::parseAttributes() {
    AttributeList attrs;
    
    while (stream.current().type == TokenType::DobleLBracket) {
        stream.advance(); 

        while (true) {
            Attribute attr;

            if (stream.current().type != TokenType::Identifier) {
                throw std::runtime_error(
                    "Expected attribute name after [[ at line " + 
                    std::to_string(stream.current().line)
                );
            }
            attr.name = std::string(stream.current().text);
            stream.advance();

            if (stream.current().type == TokenType::LParen) {
                stream.advance();
                int paren_depth = 1;
                std::string arg_text;

                while (paren_depth > 0 && !stream.isAtEnd()) {
                    if (stream.current().type == TokenType::LParen) {
                        paren_depth++;
                    } else if (stream.current().type == TokenType::RParen) {
                        paren_depth--;
                    }
                    
                    if (paren_depth > 0) {
                        arg_text += std::string(stream.current().text);
                        if (stream.current().type != TokenType::RParen) {
                            arg_text += " ";
                        }
                    }
                    stream.advance();
                }
                attr.argument = arg_text;
            }
            
            attrs.push_back(std::move(attr));
            
            if (stream.current().type == TokenType::Comma) {
                stream.advance();
                continue;
            }
            
            if (stream.current().type != TokenType::DobleRBracket) {
                throw std::runtime_error(
                    "Expected ]] to close attribute list at line " + 
                    std::to_string(stream.current().line)
                );
            }
            stream.advance();
            break;
        }
    }
    
    return attrs;
}

bool Parser::isTypeSpecifier() {
    TokenType type = stream.current().type;
    return type == TokenType::KW_void ||
           type == TokenType::KW_bool ||
           type == TokenType::KW_char ||
           type == TokenType::KW_short ||
           type == TokenType::KW_int ||
           type == TokenType::KW_long ||
           type == TokenType::KW_float ||
           type == TokenType::KW_double ||
           type == TokenType::KW_signed ||
           type == TokenType::KW_unsigned ||
           type == TokenType::KW_const ||
           type == TokenType::KW_volatile ||
           type == TokenType::KW_constexpr;
}
}   // namespace dix