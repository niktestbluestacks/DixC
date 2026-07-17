#ifndef LEXER_HPP
#define LEXER_HPP

// std
#include <cstdint>
#include <cstdlib>
#include <string_view>
// #include <unordered_map>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// dix
#include <Token.hpp>

namespace dix {

class Lexer {
 private:
  std::string_view source;
  std::size_t pos = 0;
  int line = 1;
  int column = 1;
  char current() const { return pos < source.size() ? source[pos] : '\0'; }
  char peek() const {
    return (pos + 1) < source.size() ? source[pos + 1] : '\0';
  }
  char advance() {
    char c = current();
    ++pos;
    if (c == '\n') {
      ++line;
      column = 1;
    } else {
      ++column;
    }
    return c;
  }
  void skipWhitespaceAndComments();

  Token scanIdentifierOrKeyword();
  Token scanNumber();
  Token scanString();
  Token scanOperatorOrPunctuation();
  Token scanPreprocessor();
  TokenType getTokenType(const std::string_view&);

 public:
  Lexer(std::string_view src) : source{src} {}

  Token getNextToken();
  std::vector<Token> tokenizeAll();
};

}  // namespace dix

#endif  // LEXER_HPP