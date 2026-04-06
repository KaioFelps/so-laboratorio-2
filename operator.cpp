#include "operator.h"

std::optional<Operator::Value> Operator::try_parse_operator(std::string_view entry)
{
  if (entry == "&&") return std::make_optional(Operator::ExecuteOnSuccess);
  if (entry == "||") return std::make_optional(Operator::ExecuteOnFailure);
  return std::nullopt;
}

bool Operator::might_be_operator(char c) { return c == '&' || c == '|'; }