#pragma once

#include <format>
#include <optional>
#include <print>
#include <string_view>

class Operator
{
public:
  enum Value
  {
    ExecuteOnSuccess,
    ExecuteOnFailure,
  };

  static bool might_be_operator(char c);
  static std::optional<Operator::Value> try_parse_operator(std::string_view entry);
};

template <> struct std::formatter<Operator::Value> : std::formatter<std::string_view>
{
  auto format(Operator::Value op, format_context &ctx) const
  {
    std::string_view name;
    switch (op)
    {
    case Operator::ExecuteOnSuccess:
      name = "&&";
      break;
    case Operator::ExecuteOnFailure:
      name = "||";
      break;
    }

    return std::formatter<std::string_view>::format(name, ctx);
  }
};