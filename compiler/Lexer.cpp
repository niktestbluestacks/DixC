// std
#include <cctype>
#include <stdexcept>

#include <Lexer.hpp>

namespace dix {
void Lexer::skipWhitespaceAndComments() {
    while (pos < source.size()) {
        char c = current();

        if (std::isspace(c)) {
            advance();
            continue;
        }

        if (c == '/' && peek() == '/') {
            advance(); // skip '/'
            advance(); // skip '/'
            while (pos < source.size() && current() != '\n') {
                advance();
            }
            continue;
        }

        if (c == '/' && peek() == '*') {
            advance(); // skip '/'
            advance(); // skip '*'
            
            while (pos < source.size()) {
                if (current() == '*' && peek() == '/') {
                    advance(); // skip '*'
                    advance(); // skip '/'
                    break;
                }
                advance();
            }
            
            if (pos >= source.size()) {
                throw std::runtime_error(
                    "Unterminated block comment at line "
                    + std::to_string(line)
                );
            }
            continue;
        }

        break;
    }
}

TokenType Lexer::getTokenType(const std::string_view& str) {
    if (Keywords.contains(str)) return Keywords[str];
    else if (SpecialSymbols.contains(str)) return SpecialSymbols[str];
    else if (Preprocessor.contains(str)) return Preprocessor[str];
    return TokenType::Identifier;
}

Token Lexer::scanIdentifierOrKeyword() {
    size_t start = pos;
    int start_col = column;

    while (std::isalnum(current()) || current() == '_') {
        advance();
    }

    std::string_view text = source.substr(start, pos - start);

    TokenType type = getTokenType(text);

    return { type, text, line, start_col };
}

Token Lexer::scanNumber() {
    size_t start = pos;
    int start_col = column;
    bool is_float = false;
    int base = 10;
    if (current() == '0') {
        char next = peek();
        if (next == 'x' || next == 'X') {
            base = 16;
            advance();      // to 0
            advance();      // to x
        } else if (next == 'b' || next == 'B') {
            base = 2;
            advance();      // to 0
            advance();      // to b
        } else if (std::isdigit(next)) {
            base = 8;
            advance(); // to 0
        }
    }

    bool last_was_separator = false;
    bool has_digits = false;

    auto is_digit = [base](char c) -> bool {
        if (base == 2) return c == '0' || c == '1';
        else if (base == 8) return c >= '0' && c <= '7';
        else if (base == 10) return std::isdigit(c);
        else if (base == 16) return std::isxdigit(c);
        return false;
    };

    while (pos < source.size()) {
        char c = current();

        if (c == '\'' && has_digits && !last_was_separator) {
            last_was_separator = true;
            advance();
            continue;
        }
        if (is_digit(c)) {
            has_digits = true;
            last_was_separator = false;
            advance();
            continue;
        }
        break;
    }

    if (last_was_separator) {
        throw std::runtime_error(
            "Digit separator can not be at end of number"
                + std::to_string(line)
        );
    }

    if (base == 10 || base == 16) {
       if (current() == '.' && (base == 10 || std::isxdigit(peek()))) {
            is_float = true;
            advance(); // skip '.'
            
            while (pos < source.size() && (is_digit(current()) || current() == '\'')) {
                if (current() == '\'') {
                    if (last_was_separator) {
                        throw std::runtime_error("Invalid digit separator at line " 
                                                 + std::to_string(line));
                    }
                    last_was_separator = true;
                } else {
                    last_was_separator = false;
                }
                advance();
            }
        }

        char exp_char = (base == 16) ? 'p' : 'e';
        if (current() == exp_char || current() == (exp_char - 32)) { // 'p' or 'P', 'e' or 'E'
            is_float = true;
            advance();
            if (current() == '+' || current() == '-') advance();
            
            if (!std::isdigit(current())) {
                throw std::runtime_error(
                    "Missing exponent digits at line " 
                        + std::to_string(line));
            }
            while (std::isdigit(current())) advance();
        }

        // Hex floats MUST have a p/P exponent
        if (base == 16 && is_float) {
            // Already handled above
        }
    }

    while (pos < source.size()) {
        char c = current();
        if (c == 'u' || c == 'U' || c == 'l' || c == 'L' || 
            c == 'f' || c == 'F' || c == 'd' || c == 'D') {
            if (c == 'f' || c == 'F' || c == 'd' || c == 'D') {
                is_float = true;
            }
            advance();
        } else {
            break;
        }
    }

    std::string_view text = source.substr(start, pos - start);
    TokenType type = is_float 
        ? TokenType::FloatLiteral 
        : TokenType::IntLiteral;

    return { type, text, line, start_col };
}

Token Lexer::scanString() {
    size_t start = pos;
    int start_col = column;
    char quote = current();
    advance();
     TokenType type = (quote == '"') ? TokenType::StringLiteral : TokenType::CharLiteral;

    while (pos < source.size()) {
        char c = current();

        if (c == quote) {
            advance();
            std::string_view text = source.substr(start, pos - start);
            return { type, text, line, start_col };
        }

        if (c == '\\') {
            advance();
            if (pos >= source.size()) {
                throw std::runtime_error(
                    "Unterminated escape sequence at line " 
                    + std::to_string(line));
            }
            advance();
            continue;
        }

        if (c == '\n') {
            throw std::runtime_error(
                "Newline in string literal at line " 
                + std::to_string(line)
            );
        }

        advance();
    }

    throw std::runtime_error(
        "Unterminated string literal at line " 
            + std::to_string(line));
}


Token Lexer::scanOperatorOrPunctuation() {
    size_t start = pos;
    int start_col = column;
    if (start + 2 < source.size()) {
        std::string_view three_chars = source.substr(pos, 3);
        auto it = SpecialSymbols.find(three_chars);
        if (it != SpecialSymbols.end()) {
            advance(); advance(); advance();
            return { it->second, three_chars, line, start_col };
        }
    }
    if (pos + 1 < source.size()) {
        std::string_view two_chars = source.substr(pos, 2);
        auto it = SpecialSymbols.find(two_chars);
        if (it != SpecialSymbols.end()) {
            advance(); advance();
            return { it->second, two_chars, line, start_col };
        }
    }
    std::string_view one_char = source.substr(pos, 1);
    auto it = SpecialSymbols.find(one_char);
    if (it != SpecialSymbols.end()) {
        advance();
        return { it->second, one_char, line, start_col };
    }

    throw std::runtime_error("Unexpected character '" 
        + std::string(1, current()) + 
        "' at line " + std::to_string(line) + 
        ", column " + std::to_string(start_col)
    );
}

Token Lexer::scanPreprocessor() {
    size_t start = pos;
    int start_col = column;
    
    advance();
    
    while (pos < source.size() && std::isspace(current()) && current() != '\n') {
        advance();
    }
    
    if (pos >= source.size() || current() == '\n') {
        return { TokenType::Hash, "#", line, start_col };
    }
    
    size_t directive_start = pos;
    while (pos < source.size() && std::isalpha(current())) {
        advance();
    }
    
    std::string_view directive = source.substr(directive_start, pos - directive_start);
    
    auto it = Preprocessor.find(directive);
    if (it != Preprocessor.end()) {
        std::string_view full_text = source.substr(start, pos - start);
        return { it->second, full_text, line, start_col };
    }
    
    return { TokenType::Hash, "#", line, start_col };
}

Token Lexer::getNextToken() {
    skipWhitespaceAndComments();
    if (pos >= source.size()) {
        return { TokenType::EndOfFile, "", line, column };
    }
    char c = current();

    if (c == '#') {
        return scanPreprocessor();
    }

    if (std::isalpha(c) || c == '_') {
        return scanIdentifierOrKeyword();
    }

    if (std::isdigit(c)) {
        return scanNumber();
    }
    
    if (c == '"' || c == '\'') {
        return scanString();
    }
    return scanOperatorOrPunctuation();
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokenized{};
    while (true) {
        tokenized.push_back(getNextToken());
        if (tokenized.back().type == TokenType::EndOfFile) break;
    }
    return tokenized;
}
}   // namespace dix