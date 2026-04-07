#pragma once

#include "operator.h"
#include "utils.h"
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Command
{
public:
  enum class Status
  {
    Ready,
    Running,
    SuccessfullyTerminated,
    Failed,
  };

  Command() = default;
  explicit Command(const std::vector<std::string> &args);

  /**
   * Executa o comando por meio da função `execvp`. O corrente processo será completamente
   * substituído pelo programa representado por este comando.
   */
  int run();

  const bool is_background_task() const;

  /**
   * Retorna o possível comando a ser executado de modo extraordinário à fila de execução dado o
   * estado do corrente comando. Jamais retorna um comando se o atual ainda não tiver sido
   * executado.
   */
  const std::optional<std::reference_wrapper<Command>> get_chained_command() const;

  /**
   * Sinaliza que o presente comando foi terminado (com ou sem sucesso).
   */
  void mark_as_finished(bool successfully);

  void chain_on_failure(std::shared_ptr<Command> command);

  void chain_on_success(std::shared_ptr<Command> command);

  const std::optional<std::shared_ptr<Command>> &get_success_chained_command() const;

  const std::optional<std::shared_ptr<Command>> &get_failure_chained_command() const;

  void turn_into_background_task();

  const std::vector<std::string> &get_arguments() const;

  Status get_status() const;

  std::string get_command_string() const;

private:
  Status status_ = Status::Ready;
  /**
   * Indica se este comando deve ser executado em segundo plano ou não.
   */
  bool is_background_task_ = false;
  /**
   * É o vetor de programa e seus argumentos que compõem o comando.
   */
  std::vector<std::string> arguments_ = std::vector<std::string>();
  /**
   * É o comando a ser executado imediatamente após o atual comando caso ele falhe.
   */
  std::optional<std::shared_ptr<Command>> failure_chained_command_ = std::nullopt;
  /**
   * É o comando a ser executado imediatamente após o atual comando se ele for executado com
   * sucesso.
   */
  std::optional<std::shared_ptr<Command>> success_chained_command_ = std::nullopt;

  const std::string &get_program() const;
  const std::vector<char *> get_constant_arguments() const;
};

template <> struct std::formatter<Command> : std::formatter<std::string>
{
  auto format(const Command &cmd, format_context &ctx) const
  {
    std::string args_str = "(";
    const auto &args = cmd.get_arguments();

    for (size_t i = 0; i < args.size(); ++i)
    {
      args_str += std::format("\"{}\"", args[i]);
      if (i < args.size() - 2) args_str += ", ";
    }
    args_str += ")";

    std::string bg_str = cmd.is_background_task() ? "true" : "false";

    std::string failure_str = "{}";
    if (const auto &failure_cmd = cmd.get_failure_chained_command())
    {
      failure_str = std::format("{}", **failure_cmd);
    }

    std::string success_str = "{}";
    if (const auto &success_cmd = cmd.get_success_chained_command())
    {
      success_str = std::format("{}", **success_cmd);
    }

    return std::formatter<std::string>::format(
        std::format("C({}, {}, {}, {})", args_str, bg_str, failure_str, success_str), ctx);
  }
};

template <> struct std::formatter<Command::Status> : std::formatter<std::string_view>
{
  auto format(const Command::Status &status, format_context &ctx) const
  {
    std::string_view name;
    switch (status)
    {
    case Command::Status::Failed:
      name = "Concluído com erro";
      break;
    case Command::Status::Ready:
      name = "Preparado";
      break;
    case Command::Status::Running:
      name = "Em execução";
      break;
    case Command::Status::SuccessfullyTerminated:
      name = "Concluído com sucesso";
      break;
    }

    return std::formatter<std::string_view>::format(name, ctx);
  }
};