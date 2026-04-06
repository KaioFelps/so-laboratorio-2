/**
 * Leia o README.md deste diretório.
 */

#include "command.h"
#include "operator.h"
#include "parser.h"
#include <iostream>
#include <print>
#include <string_view>
#include <vector>

/**
 * Inicia o processo em segundo plano e imprime sua posição nas tarefas a serem executadas em plano
 * de fundo e seu PID no terminal.
 */
void spawn_background_task() {}

bool is_debug(int args_len, char *args[])
{
  if (args_len <= 1) return false;

  const auto flag = std::string_view(args[1]);
  const auto is_debug_flag = flag == "-d" || flag == "--debug" || flag == "-D";
  return is_debug_flag;
}

int main(int args_len, char *args[])
{
  if (is_debug(args_len, args)) debug_loop();

  return 0;
}