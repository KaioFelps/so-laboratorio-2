/**
 * Leia o README.md deste diretório.
 */

#include "command.h"
#include "operator.h"
#include "parser.h"
#include <iostream>
#include <print>
#include <vector>

/**
 * Inicia o processo em segundo plano e imprime sua posição nas tarefas a serem executadas em plano
 * de fundo e seu PID no terminal.
 */
void spawn_background_task() {}

int main()
{

  while (true)
  {
    auto line = std::string();
    getline(std::cin, line);

    auto tokens_result = Lexer().tokenize(line);

    if (!tokens_result)
    {
      std::println("{}", tokens_result.error().what());
      exit(1);
    }

    auto tokens = *tokens_result;

    while (tokens.size())
    {
      auto &token = tokens.front();

      std::visit(
          match{
              [](const std::vector<std::string> &args)
              {
                std::println("Vetor de argumentos:");
                for (auto &arg : args)
                {
                  println("\t{}", arg);
                }
              },
              [](const Operator::Value op) { std::println("Operador({})", op); },
              [](const Separator::Value separator) { std::println("Separador({})", separator); },
              [](const Parenthesis::Value parenthesis)
              { std::println("Parenthesis({})", parenthesis); },
          },
          token);

      tokens.pop();
    }
  }

  return 0;
}