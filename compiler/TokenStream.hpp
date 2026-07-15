#ifndef TOKEN_STREAM_WRAPPER_HPP
#define TOKEN_STREAM_WRAPPER_HPP

// dix
#include <Lexer.hpp>

// std
#include <stdexcept>
#include <string_view>
#include <vector>

namespace dix {
class TokenStream {
private:
    std::vector<Token> tokens;
    size_t pos = 0;
public: 
    TokenStream(std::string_view source) {
        Lexer lexer{source};
        while (true) {
            Token token = lexer.getNextToken();
            tokens.push_back(token);
            if (token.type == TokenType::EndOfFile) break;
        }
    }

    const Token& current() const {
        if (pos >= tokens.size()) {
            return tokens.back();
        }
        return tokens[pos];
    }

    const Token& peek(int offset = 1) const {
        size_t idx = pos + offset;
        if (idx >= tokens.size()) {
            return tokens.back(); // Return EOF
        }
        return tokens[idx];
    }

    Token advance() {
        Token token = current();
        if (pos < tokens.size() - 1) {
            pos++;
        }
        return token;
    }

    bool match(TokenType type) {
        if (current().type == type) {
            advance();
            return true;
        }
        return false;
    }

    Token expect(TokenType type, const std::string& error_msg) {
        if (current().type == type) {
            return advance();
        }
        throw std::runtime_error(
            error_msg + " at line " + std::to_string(current().line) +
            ", column " + std::to_string(current().column) +
            " (got unknown token)"
        );
    }

    bool isAtEnd() const {
        return current().type == TokenType::EndOfFile;
    }

    size_t getPosition() const {
        return pos;
    }

    void setPosition(size_t new_pos) {
        pos = new_pos;
    }
};
}   // namespace dix

#endif // TOKEN_STREAM_WRAPPER_HPP