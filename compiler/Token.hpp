#ifndef TOKEN_HPP
#define TOKEN_HPP

// std
#include <cstdint>
#include <map>
#include <string_view>

namespace dix {

enum class TokenType : std::uint8_t {

  // ----------------------------- Special
  EndOfFile,
  Invalid,

  // ----------------------------- Literals
  Identifier,
  IntLiteral,
  FloatLiteral,
  CharLiteral,
  StringLiteral,

  // ----------------------------- Types & Modifiers
  KW_void,
  KW_bool,
  KW_char,
  KW_short,
  KW_int,
  KW_long,
  KW_float,
  KW_double,
  KW_signed,
  KW_unsigned,
  KW_typeof,
  KW_typeof_unqual,
  KW_constexpr,
  KW_true,
  KW_false,
  KW_nullptr,

  // ----------------------------- Storage & Qualifiers
  KW_auto,
  KW_register,
  KW_static,
  KW_extern,
  KW_thread_local,
  KW_const,
  KW_volatile,
  KW_restrict,
  KW_inline,
  KW_typedef,
  KW_struct,
  KW_union,
  KW_enum,

  // ----------------------------- Contorol Flow & Statements
  KW_if,
  KW_else,
  KW_switch,
  KW_case,
  KW_default,
  KW_while,
  KW_do,
  KW_for,
  KW_break,
  KW_continue,
  KW_return,
  KW_goto,
  KW_static_assert,

  // ----------------------------- Operators & Expressions
  KW_sizeof,
  KW_alignof,
  KW_alignas,

  // ----------------------------- Operators
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Ampersand,
  Pipe,
  Caret,
  Tilde,
  Exclaim,
  Less,
  Greater,
  Equal,

  // ----------------------------- Multi-charecter operators
  PlusPlus,
  MinusMinus,
  LShift,
  RShift,
  PlusEqual,
  MinusEqual,
  StarEqual,
  SlashEqual,
  PercentEqual,
  AmpEqual,
  PipeEqual,
  CaretEqual,
  LShiftEqual,
  RShiftEqual,
  EqualEqual,
  ExclaimEqual,
  LessEqual,
  GreaterEqual,
  AmpAmp,
  PipePipe,
  Arrow,
  Dot,
  Question,
  Colon,
  Ellipsis,

  // ----------------------------- Punctiation
  LParen,
  RParen,  // ( )
  LBrace,
  RBrace,  // { }
  LBracket,
  RBracket,       // [ ]
  DobleLBracket,  // [[
  DobleRBracket,  // ]]
  Semicolon,
  Comma,  // ; ,
  Hash,   // # (for preprocessor)

  // ----------------------------- Preprocessor
  PP_include,
  PP_define,
  PP_undef,
  PP_if,
  PP_ifdef,
  PP_ifndef,
  PP_elif,
  PP_else,
  PP_endif,
  PP_line,
  PP_error,
  PP_warning,
  PP_pragma,
  PP_embed,
  PP_HashHash,
  PP_Stringify
};

inline static std::map<std::string_view, TokenType> Keywords = {
    {"void", TokenType::KW_void},
    {"bool", TokenType::KW_bool},
    {"char", TokenType::KW_char},
    {"short", TokenType::KW_short},
    {"int", TokenType::KW_int},
    {"long", TokenType::KW_long},
    {"float", TokenType::KW_float},
    {"double", TokenType::KW_double},
    {"signed", TokenType::KW_signed},
    {"unsigned", TokenType::KW_unsigned},
    {"typeof", TokenType::KW_typeof},
    {"typeof_unqual", TokenType::KW_typeof_unqual},
    {"constexpr", TokenType::KW_constexpr},
    {"true", TokenType::KW_true},
    {"false", TokenType::KW_false},
    {"nullptr", TokenType::KW_nullptr},
    {"auto", TokenType::KW_auto},
    {"register", TokenType::KW_register},
    {"static", TokenType::KW_static},
    {"extern", TokenType::KW_extern},
    {"thread_local", TokenType::KW_thread_local},
    {"const", TokenType::KW_const},
    {"volatile", TokenType::KW_volatile},
    {"restrict", TokenType::KW_restrict},
    {"inline", TokenType::KW_inline},
    {"typedef", TokenType::KW_typedef},
    {"struct", TokenType::KW_struct},
    {"union", TokenType::KW_union},
    {"enum", TokenType::KW_enum},
    {"if", TokenType::KW_if},
    {"else", TokenType::KW_else},
    {"switch", TokenType::KW_switch},
    {"case", TokenType::KW_case},
    {"default", TokenType::KW_default},
    {"while", TokenType::KW_while},
    {"do", TokenType::KW_do},
    {"for", TokenType::KW_for},
    {"break", TokenType::KW_break},
    {"continue", TokenType::KW_continue},
    {"return", TokenType::KW_return},
    {"goto", TokenType::KW_goto},
    {"static_assert", TokenType::KW_static_assert},
    {"sizeof", TokenType::KW_sizeof},
    {"alignof", TokenType::KW_alignof},
    {"alignas", TokenType::KW_alignas}};

inline static std::map<std::string_view, TokenType> SpecialSymbols = {
    {"+", TokenType::Plus},
    {"-", TokenType::Minus},
    {"*", TokenType::Star},
    {"/", TokenType::Slash},
    {"%", TokenType::Percent},
    {"&", TokenType::Ampersand},
    {"|", TokenType::Pipe},
    {"^", TokenType::Caret},
    {"~", TokenType::Tilde},
    {"!", TokenType::Exclaim},
    {"<", TokenType::Less},
    {">", TokenType::Greater},
    {"=", TokenType::Equal},
    {"++", TokenType::PlusPlus},
    {"--", TokenType::MinusMinus},
    {"<<", TokenType::LShift},
    {">>", TokenType::RShift},
    {"+=", TokenType::PlusEqual},
    {"-=", TokenType::MinusEqual},
    {"*=", TokenType::StarEqual},
    {"/=", TokenType::SlashEqual},
    {"%=", TokenType::PercentEqual},
    {"&=", TokenType::AmpEqual},
    {"|=", TokenType::PipeEqual},
    {"^=", TokenType::CaretEqual},
    {"<<=", TokenType::LShiftEqual},
    {">>=", TokenType::RShiftEqual},
    {"==", TokenType::EqualEqual},
    {"!=", TokenType::ExclaimEqual},
    {"<=", TokenType::LessEqual},
    {">=", TokenType::GreaterEqual},
    {"&&", TokenType::AmpAmp},
    {"||", TokenType::PipePipe},
    {"->", TokenType::Arrow},
    {".", TokenType::Dot},
    {"?", TokenType::Question},
    {":", TokenType::Colon},
    {"...", TokenType::Ellipsis},
    {"(", TokenType::LParen},
    {")", TokenType::RParen},
    {"{", TokenType::LBrace},
    {"}", TokenType::RBrace},
    {"[", TokenType::LBracket},
    {"]", TokenType::RBracket},
    {"[[", TokenType::DobleLBracket},
    {"]]", TokenType::DobleRBracket},
    {";", TokenType::Semicolon},
    {",", TokenType::Comma},
    {"#", TokenType::Hash},
    {"##", TokenType::PP_HashHash}};

inline static std::map<std::string_view, TokenType> Preprocessor = {
    {"include", TokenType::PP_include}, {"define", TokenType::PP_define},
    {"undef", TokenType::PP_undef},     {"if", TokenType::PP_if},
    {"ifdef", TokenType::PP_ifdef},     {"ifndef", TokenType::PP_ifndef},
    {"elif", TokenType::PP_elif},       {"else", TokenType::PP_else},
    {"endif", TokenType::PP_endif},     {"line", TokenType::PP_line},
    {"error", TokenType::PP_error},     {"warning", TokenType::PP_warning},
    {"pragma", TokenType::PP_pragma},   {"embed", TokenType::PP_embed},
    // {"##", TokenType::PP_HashHash},
    // {"#", TokenType::PP_Stringify}
};

struct Token {
  TokenType type;
  std::string_view text;
  int line;
  int column;
};
}  // namespace dix

#endif  // TOKEN_HPP