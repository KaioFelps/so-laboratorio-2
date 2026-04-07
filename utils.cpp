#include "utils.h"
#include "parser.h"
#include <iostream>
#include <print>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <unordered_map>

using namespace std;

static bool check_process_has_terminated(const int status, bool &successfully)
{
  const auto has_exited_successfully = WIFEXITED(status);
  if (has_exited_successfully)
  {
    const auto is_failure_signal = WEXITSTATUS(status) == 1;
    successfully = !is_failure_signal;
    return true;
  }

  const auto has_been_terminated = WIFSIGNALED(status);
  if (has_been_terminated)
  {
    println("Processo filho morto pelo sinal {}.", WTERMSIG(status));
    successfully = false;
    return true;
  }

  const auto has_frozen = WIFSTOPPED(status);
  if (has_frozen)
  {
    println("O processo filho foi pausado.");
    return false;
  }

  const auto has_unpaused = WIFCONTINUED(status);
  if (has_unpaused)
  {
    println("O processo filho foi retomado.");
    return false;
  }

  return false;
}

#define WAIT_BLOCKING_OPTIONS WUNTRACED | WCONTINUED
static bool block_on_proc(pid_t pid)
{
  auto successfully_terminated = false;
  auto status = 0;

  while (true)
  {
    // Se usássemos `wait` ao invés de `waitpid` o resultado seria parecido, mas ele só
    // desbloquearia o processo pai quando o processo filho fosse terminado e, nesse caso, não
    // seríamos informados sobre quaisquer outros status atribuídos ao processo filho. O `waitpid`
    // recebe diferentes tipos de sinais, como de pausa e continuação. Por isso, não podemos
    // simplesmente chamar `waitpid` uma vez e assumir que o processo foi encerrado, precisamos
    // ficar em um loop monitorando os sinais recebidos até que recebamos um dos sinais que indicam
    // encerramento.
    const auto waitpid_signal = waitpid(pid, &status, WAIT_BLOCKING_OPTIONS);
    const auto could_not_monitor_child_process = waitpid_signal == -1;
    if (could_not_monitor_child_process)
    {
      println(stderr, "Não foi possível rastrear o estado do programa executado.");
      exit(1);
    }

    if (check_process_has_terminated(status, successfully_terminated)) break;
  }

  return successfully_terminated;
}

pid_t spawn_task_async(Command &command)
{
  const auto child_pid = fork();
  const auto is_child_process = child_pid == 0;

  if (is_child_process)
  {
    command.run();
    // Código caso não seja possível executar o comando no fork, só é alcançado se o `execvp`
    // interno falhar
    const auto command_str = command.get_command_string();
    println(stderr, "Não foi possível executar o comando `{}` em segundo plano", command_str);
    exit(1);
  }

  return child_pid;
}

pid_t spawn_task_blocking(Command &command)
{
  const auto child_pid = fork();
  const auto is_child_process = child_pid == 0;

  if (is_child_process)
  {
    command.run();
    // Código caso não seja possível executar o comando no fork, só é alcançado se o `execvp`
    // interno falhar
    println(stderr, "Não foi possível executar o comando `{}`", command.get_command_string());
    exit(1);
  }

  const auto has_finished_successfully = block_on_proc(child_pid);
  command.mark_as_finished(has_finished_successfully);
  return child_pid;
}

static void
clean_finished_background_tasks(unordered_map<pid_t, pair<size_t, string_view>> &background_tasks)
{
  auto pids_to_delete = std::vector<pid_t>();

  for (auto &[process_id, data] : background_tasks)
  {
    auto status = 0;
    auto wait_result = waitpid(process_id, &status, WNOHANG);
    const auto has_finished = wait_result != 0;

    if (!has_finished) continue;

    const auto &[process_index, command_string] = data;

    string info = status == 0 ? "Concluído" : "Fim da execução com status " + status;
    println("[{}]+\t{:<30}\t{}", process_index, info, command_string);

    pids_to_delete.push_back(process_id);
  }

  for (auto process_id : pids_to_delete)
  {
    background_tasks.erase(process_id);
  }
}

void loop()
{
  auto background_tasks_pids = unordered_map<pid_t, pair<size_t, string_view>>();

  while (true)
  {
    auto background_tasks_counter = 0;

    auto command_line = string();
    getline(cin, command_line);

    auto queue_result = parse_commands(command_line);

    if (!queue_result)
    {
      println(stderr, "{}", queue_result.error().what());
      exit(1);
    }

    auto queue = queue_result.value();

    while (!queue.empty())
    {
      auto command = move(queue.front());
      queue.pop();

      bool has_chained_command = true;
      while (has_chained_command)
      {
        if (command.is_background_task())
        {
          auto child_pid = spawn_task_async(command);
          auto child_id_in_command_line = ++background_tasks_counter;
          background_tasks_pids[child_pid] =
              make_pair(child_id_in_command_line, command.get_command_string());

          println("[{}] {}", child_id_in_command_line, child_pid);
        }
        else
        {
          spawn_task_blocking(command);
        }

        if (const auto &chained_command = command.get_chained_command())
        {
          // Aqui, criamos uma cópia do `chained_command`. O comando encadeado está guardado,
          // internamente (no `command`), dentro de um ponteiro compartilhado. Na linha seguinte, o
          // `command` recebe outro valor e essa instância sairá do escopo e será deletada.
          //
          // O contador do ponteiro compartilhado talvez chegue a 0 com a remoção dessa instância e,
          // portanto, remova o comando aninhado original da heap. Como fizemos uma cópia, não
          // seremos afetados. Essa brincadeira só funciona porque nenhum comando depende do estado
          // interno do outro, mesmo que tenha um ponteiro para este.
          command = *chained_command;
          continue;
        }

        has_chained_command = false;
      }
    }

    clean_finished_background_tasks(background_tasks_pids);
  }
}

void debug_loop()
{
  while (true)
  {
    auto line = string();
    getline(cin, line);

    auto tokens_result = Lexer().tokenize(line);

    if (!tokens_result)
    {
      println(stderr, "{}", tokens_result.error().what());
      exit(1);
    }

    println("Pilha de tokens:");
    auto tokens = *tokens_result;
    while (tokens.size())
    {
      auto &token = tokens.front();

      visit(
          match{
              [](const vector<string> &args)
              {
                println("Vetor de argumentos:");
                for (auto &arg : args)
                {
                  println("\t{}", arg);
                }
              },
              [](const Operator::Value op) { println("Operador({})", op); },
              [](const Separator::Value separator) { println("Separador({})", separator); },
              [](const Parenthesis::Value parenthesis) { println("Parenthesis({})", parenthesis); },
          },
          token);

      tokens.pop();
    }

    auto parser_result = parse_commands(line);

    if (!parser_result)
    {
      println(stderr, "{}", parser_result.error().what());
      exit(1);
    }

    println("\nFila de comandos:");
    auto commands = parser_result.value();
    while (!commands.empty())
    {
      auto command = commands.front();
      commands.pop();
      println("\t - {},", command);
    }
  }
}