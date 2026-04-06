#pragma once

#include "operator.h"
#include "utils.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Command
{
public:
  enum class State
  {
    Ready,
    Running,
    SuccessfullyTerminated,
    Failed,
  };

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

private:
  State state_ = State::Ready;
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
  std::optional<std::shared_ptr<Command>> failure_chained_command;
  /**
   * É o comando a ser executado imediatamente após o atual comando se ele for executado com
   * sucesso.
   */
  std::optional<std::shared_ptr<Command>> success_chained_command;

  const std::string &get_program() const;
  const std::vector<char *> get_constant_arguments() const;
};
