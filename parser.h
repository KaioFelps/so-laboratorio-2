#pragma once

#include "command.h"
#include "errors.h"
#include "operator.h"

#include <expected>
#include <format>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct Separator
{
  enum Value
  {
    Sequential,
    Background,
  };

  static bool might_be_separator(char c);
  static std::optional<Value> try_parse_separator(std::string_view entry);
};

template <> struct std::formatter<Separator::Value> : std::formatter<std::string_view>
{
  auto format(Separator::Value op, format_context &ctx) const
  {
    std::string_view name;
    switch (op)
    {
    case Separator::Background:
      name = "&";
      break;
    case Separator::Sequential:
      name = ";";
      break;
    }

    return std::formatter<std::string_view>::format(name, ctx);
  }
};

struct Parenthesis
{
public:
  enum Value
  {
    Opening,
    Closing,
  };

  static std::optional<Value> try_parse_parenthesis(std::string_view entry);
  static std::optional<Value> try_parse_parenthesis(char entry);
};

template <> struct std::formatter<Parenthesis::Value> : std::formatter<std::string_view>
{
  auto format(Parenthesis::Value op, format_context &ctx) const
  {
    std::string_view name;
    switch (op)
    {
    case Parenthesis::Opening:
      name = "(";
      break;
    case Parenthesis::Closing:
      name = ")";
      break;
    }

    return std::formatter<std::string_view>::format(name, ctx);
  }
};

struct Quote
{
  enum Value
  {
    Single,
    Double,
  };

  static std::optional<Value> try_parse_quote(std::string_view entry);
  static std::optional<Value> try_parse_quote(char entry);
};

using Token =
    std::variant<std::vector<std::string>, Operator::Value, Separator::Value, Parenthesis::Value>;

class Lexer
{
private:
  std::optional<Quote::Value> wrapping_quote;
  std::queue<Token> tokens;
  std::string chars_buffer;
  std::vector<std::string> args_buffer;
  std::string operators_buffer;
  bool shall_escape = false;
  bool has_entered_quoted_empty_string = false;

  bool is_in_quotes();
  void flush_args();
  std::expected<void, SyntaxError> flush_operators();
  void flush_chars_buffer();
  void maybe_insert_empty_string_argument();

public:
  // o && indica que esse método só pode ser chamado em r-values (valores que estão prestes a serem
  // dropados)
  std::expected<std::queue<Token>, SyntaxError> tokenize(std::string_view command_line) &&;
};

class Parser
{
public:
  static std::queue<Command> parse_commands(std::string_view command_line);
};