#include "parser.h"
#include <memory>

RawEntry RawEntry::make_command(std::vector<std::string> argument)
{
  RawEntry entry;
  entry.args = std::move(argument);
  return entry;
}

RawEntry RawEntry::make_group(std::vector<RawEntry> group)
{
  RawEntry entry;
  entry.group = std::move(group);
  return entry;
}

static std::expected<std::vector<RawEntry>, SyntaxError> flatten(std::queue<Token> &tokens,
                                                                 bool inside_parentheses);

static std::expected<std::queue<Command>, SyntaxError>
build_queue(const std::vector<RawEntry> &entries, std::size_t from = 0);

static std::optional<Token> collect_connector(std::queue<Token> &tokens)
{
  if (tokens.empty()) return std::nullopt;
  const Token &next_token = tokens.front();

  if (std::holds_alternative<Operator::Value>(next_token) ||
      std::holds_alternative<Separator::Value>(next_token))
  {
    auto connector = next_token;
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

static void turn_command_tree_into_bg_task(Command &cmd)
{
  auto success_command = cmd.get_success_chained_command();
  if (!success_command)
  {
    cmd.turn_into_background_task();
    return;
  }

  turn_command_tree_into_bg_task(**success_command);

  if (auto failure_command = cmd.get_failure_chained_command())
  {
    turn_command_tree_into_bg_task(**failure_command);
  }
}

static void apply_operator_to_command_tree(Command &cmd, Operator::Value op,
                                           const std::shared_ptr<Command> &target)
{
  auto success_command = cmd.get_success_chained_command();
  auto failure_command = cmd.get_failure_chained_command();

  if (op == Operator::ExecuteOnSuccess)
  {
    if (success_command)
    {
      apply_operator_to_command_tree(**success_command, op, target);
    }
    else
    {
      cmd.chain_on_success(target);
    }

    if (failure_command)
    {
      apply_operator_to_command_tree(**failure_command, op, target);
    }

    return;
  }

  if (failure_command)
  {
    apply_operator_to_command_tree(**failure_command, op, target);
  }
  else
  {
    cmd.chain_on_failure(target);
  }

  if (success_command) apply_operator_to_command_tree(**success_command, op, target);
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
      if (*sep == Separator::Background)
      {
        turn_command_tree_into_bg_task(cmd);

        std::queue<Command> new_extras;
        while (!extras.empty())
        {
          Command extra = extras.front();
          extras.pop();
          turn_command_tree_into_bg_task(extra);
          new_extras.push(std::move(extra));
        }

        result.push(std::move(cmd));
        while (!new_extras.empty())
        {
          result.push(new_extras.front());
          new_extras.pop();
        }
      }
      else
      {
        result.push(std::move(cmd));
        flush_extras();
      }
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

      apply_operator_to_command_tree(cmd, *op, chained);

      std::queue<Command> new_extras;
      while (!extras.empty())
      {
        Command extra = extras.front();
        extras.pop();
        apply_operator_to_command_tree(extra, *op, chained);
        new_extras.push(std::move(extra));
      }

      result.push(std::move(cmd));
      while (!new_extras.empty())
      {
        result.push(new_extras.front());
        new_extras.pop();
      }

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

std::expected<std::queue<Command>, SyntaxError> parse_commands(std::string_view command_line)
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