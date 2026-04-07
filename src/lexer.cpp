#include "parser.h"

std::expected<std::queue<Token>, SyntaxError> Lexer::tokenize(std::string_view command_line) &&
{
  for (auto c : command_line)
  {
    const auto as_quote = Quote::try_parse_quote(c);

    if (as_quote && !this->shall_escape)
    {
      if (!this->is_in_quotes())
      {
        this->wrapping_quote = as_quote;
      }
      else if (*as_quote == *this->wrapping_quote)
      {
        this->wrapping_quote = std::nullopt;
      }

      const auto user_entered_empty_string = !this->is_in_quotes() && this->chars_buffer.empty();
      if (user_entered_empty_string) this->has_entered_quoted_empty_string = true;

      this->flush_operators();
      continue;
    }

    if (this->is_in_quotes())
    {
      if (!this->shall_escape && c == '\\')
      {
        this->shall_escape = true;
        continue;
      }

      this->chars_buffer.push_back(c);
      this->shall_escape = false;
      continue;
    }

    this->shall_escape = false;
    this->maybe_insert_empty_string_argument();

    const auto is_delimiter = c == ' ' || c == '\t';
    if (is_delimiter)
    {
      this->flush_operators();
      this->flush_chars_buffer();

      continue;
    }

    if (Operator::might_be_operator(c) || Separator::might_be_separator(c))
    {
      this->maybe_insert_empty_string_argument();
      this->operators_buffer.push_back(c);
      this->flush_args();
      continue;
    }

    auto flush_result = this->flush_operators();
    if (!flush_result) return std::unexpected(flush_result.error());

    if (const auto as_parenthesis = Parenthesis::try_parse_parenthesis(c))
    {
      this->maybe_insert_empty_string_argument();
      this->flush_args();
      this->tokens.push(*as_parenthesis);
      continue;
    }

    this->chars_buffer.push_back(c);
  }

  this->maybe_insert_empty_string_argument();

  const auto flush_result = this->flush_operators();
  if (!flush_result) return std::unexpected(flush_result.error());

  this->flush_args();

  if (this->is_in_quotes())
  {
    return std::unexpected(SyntaxError("Erro de sintaxe: aspas não fechadas ao final da linha"));
  }

  return std::move(this->tokens);
}

void Lexer::flush_args()
{
  this->flush_chars_buffer();

  if (this->args_buffer.empty()) return;

  this->tokens.push(std::move(this->args_buffer));
  this->args_buffer.clear();
}

std::expected<void, SyntaxError> Lexer::flush_operators()
{
  if (this->operators_buffer.empty()) return {};

  if (const auto as_operator = Operator::try_parse_operator(this->operators_buffer))
  {
    this->tokens.push(*as_operator);
    this->operators_buffer.clear();
    return {};
  }

  if (const auto as_separator = Separator::try_parse_separator(this->operators_buffer))
  {
    this->tokens.push(*as_separator);
    this->operators_buffer.clear();
    return {};
  }

  const auto error = std::unexpected(
      SyntaxError("Erro encontrado próximo ao token inesperado `" + this->operators_buffer + "`"));

  return error;
}

void Lexer::flush_chars_buffer()
{
  if (this->chars_buffer.empty()) return;

  this->args_buffer.push_back(std::move(this->chars_buffer));
  this->chars_buffer.clear();
}

bool Lexer::is_in_quotes() { return this->wrapping_quote.has_value(); }

void Lexer::maybe_insert_empty_string_argument()
{

  if (this->has_entered_quoted_empty_string) this->args_buffer.push_back("");
  this->has_entered_quoted_empty_string = false;
}
