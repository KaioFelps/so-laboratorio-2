#include "parser.h"
#include <memory>

struct RawEntry
{
  std::optional<std::vector<std::string>> args;
  std::optional<std::vector<RawEntry>> group;

  std::optional<Token> connector;

  static RawEntry make_command(std::vector<std::string> a)
  {
    RawEntry e;
    e.args = std::move(a);
    return e;
  }

  static RawEntry make_group(std::vector<RawEntry> g)
  {
    RawEntry e;
    e.group = std::move(g);
    return e;
  }
};

static std::expected<std::vector<RawEntry>, SyntaxError> flatten(std::queue<Token> &tokens,
                                                                 bool inside_parentheses);

static std::expected<std::queue<Command>, SyntaxError>
build_queue(const std::vector<RawEntry> &entries, std::size_t from = 0);

static std::optional<Token> collect_connector(std::queue<Token> &tokens)
{
  if (tokens.empty()) return std::nullopt;
  const Token &next = tokens.front();
  if (std::holds_alternative<Operator::Value>(next) ||
      std::holds_alternative<Separator::Value>(next))
  {
    Token connector = next;
    tokens.pop();
    return connector;
  }
  return std::nullopt;
}

static std::expected<std::vector<RawEntry>, SyntaxError> flatten(std::queue<Token> &tokens,
                                                                 bool inside_parentheses)
{
  std::vector<RawEntry> entries;

  while (!tokens.empty())
  {
    Token token = tokens.front();
    tokens.pop();

    if (auto *parenthesis = std::get_if<Parenthesis::Value>(&token))
    {
      if (*parenthesis == Parenthesis::Closing)
      {
        if (!inside_parentheses)
        {
          return std::unexpected(
              SyntaxError("Erro de sintaxe: fechamento inesperado de grupo de parênteses"));
        }

        return entries;
      }

      auto inner_command = flatten(tokens, true);
      if (!inner_command) return std::unexpected(inner_command.error());

      if (inner_command->empty())
      {
        return std::unexpected(
            SyntaxError("Erro de sintaxe: grupo de parênteses vazio inesperado"));
      }

      auto group_entry = RawEntry::make_group(std::move(*inner_command));
      group_entry.connector = collect_connector(tokens);
      entries.push_back(std::move(group_entry));
      continue;
    }

    if (auto *args = std::get_if<std::vector<std::string>>(&token))
    {
      RawEntry entry = RawEntry::make_command(std::move(*args));
      entry.connector = collect_connector(tokens);
      entries.push_back(std::move(entry));
      continue;
    }

    return std::unexpected(
        SyntaxError("Erro de sintaxe: encontrado operadores binários sem expressões à esquerda"));
  }

  if (inside_parentheses)
    return std::unexpected(SyntaxError("Erro de sintaxe: um grupo de parênteses não foi fechado"));

  return entries;
}

static std::expected<std::pair<Command, std::queue<Command>>, SyntaxError>
resolve_entry(const RawEntry &entry)
{
  if (entry.args.has_value())
  {
    return std::make_pair(Command(*entry.args), std::queue<Command>{});
  }

  if (entry.group.has_value())
  {
    auto sub_command = build_queue(*entry.group);
    if (!sub_command) return std::unexpected(sub_command.error());

    if (sub_command->empty())
      return std::unexpected(SyntaxError("Erro de sintaxe: grupo de comandos vazio encontrado"));

    Command head = sub_command->front();
    sub_command->pop();
    return std::make_pair(std::move(head), std::move(*sub_command));
  }

  return std::unexpected(SyntaxError("Entrada inválida: não há argumentos nem grupos de comandos"));
}

static std::expected<std::queue<Command>, SyntaxError>
build_queue(const std::vector<RawEntry> &entries, std::size_t from)
{
  std::queue<Command> result;

  for (std::size_t i = from; i < entries.size();)
  {
    const RawEntry &entry = entries[i];

    auto resolved = resolve_entry(entry);
    if (!resolved) return std::unexpected(resolved.error());

    auto &[cmd, extras] = *resolved;

    auto flush_extras = [&]()
    {
      while (!extras.empty())
      {
        result.push(extras.front());
        extras.pop();
      }
    };

    if (!entry.connector.has_value())
    {
      result.push(std::move(cmd));
      flush_extras();
      ++i;
      continue;
    }

    const Token &connector = *entry.connector;

    if (auto *sep = std::get_if<Separator::Value>(&connector))
    {
      if (*sep == Separator::Background) cmd.turn_into_background_task();

      result.push(std::move(cmd));
      flush_extras();
      ++i;
      continue;
    }

    if (auto *op = std::get_if<Operator::Value>(&connector))
    {
      if (i + 1 >= entries.size())
      {
        return std::unexpected(
            SyntaxError(std::format("Operador `{}` sem comando à direita", *op)));
      }

      auto sub_command = build_queue(entries, i + 1);
      if (!sub_command) return std::unexpected(sub_command.error());

      if (sub_command->empty())
      {
        return std::unexpected(SyntaxError(std::format("Expressão vazia após operador `{}`", *op)));
      }

      auto chained = std::make_shared<Command>(sub_command->front());
      sub_command->pop();

      if (*op == Operator::ExecuteOnSuccess)
      {
        cmd.chain_on_success(chained);
      }
      else
      {
        cmd.chain_on_failure(chained);
      }

      result.push(std::move(cmd));
      flush_extras();

      while (!sub_command->empty())
      {
        result.push(sub_command->front());
        sub_command->pop();
      }

      i = entries.size();
      continue;
    }

    ++i;
  }

  return result;
}

std::expected<std::queue<Command>, SyntaxError>
Parser::parse_commands(std::string_view command_line)
{
  auto token_result = Lexer().tokenize(command_line);
  if (!token_result) return std::unexpected(token_result.error());

  auto tokens = std::move(*token_result);
  if (tokens.empty()) return std::queue<Command>();

  auto raw_entries = flatten(tokens, false);
  if (!raw_entries) return std::unexpected(raw_entries.error());

  if (raw_entries->empty()) return std::queue<Command>();

  return build_queue(*raw_entries);
}
