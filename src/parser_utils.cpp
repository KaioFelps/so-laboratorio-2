#include "parser.h"

bool Separator::might_be_separator(char c) { return c == ';' || c == '&'; }

std::optional<Separator::Value> Separator::try_parse_separator(std::string_view entry)
{
  if (entry == "&") return std::make_optional(Separator::Background);
  if (entry == ";") return std::make_optional(Separator::Sequential);
  return std::nullopt;
}

std::optional<Parenthesis::Value> Parenthesis::try_parse_parenthesis(std::string_view entry)
{
  if (entry.empty()) return std::nullopt;
  return try_parse_parenthesis(entry.front());
}

std::optional<Parenthesis::Value> Parenthesis::try_parse_parenthesis(char entry)
{
  if (entry == '(') return std::make_optional(Parenthesis::Opening);
  if (entry == ')') return std::make_optional(Parenthesis::Closing);
  return std::nullopt;
}

std::optional<Quote::Value> Quote::try_parse_quote(std::string_view entry)
{
  if (entry.empty()) return std::nullopt;
  return try_parse_quote(entry.front());
}

std::optional<Quote::Value> Quote::try_parse_quote(char entry)
{
  if (entry == '"') return std::make_optional(Quote::Double);
  if (entry == '\'') return std::make_optional(Quote::Single);
  return std::nullopt;
}
