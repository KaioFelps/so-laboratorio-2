/**
 * O programa lida com três tipos de dados principais:
 * - Argumentos - são strings que formam um comando;
 * - Separadores - são entradas que separam um comando do outro (`&` ou `;`):
 *   - `&`: o programa anterior é executado em plano de fundo;
 *   - `;`: o próximo programa é executado independentemente do sucesso ou fracasso do anterior.
 * - Operadores - são uma variação dos separadores: encerram um comando, mas ao invés de iniciar um
 *   novo comando na fila de execução, o próximo comando é salvo na estrutura do comando atual
 *   dada a operação lógica utilizada:
 *   - `&&`: o próximo programa será executado se, e somente se, o presente for executado com
 *           sucesso;
 *   - `||`: o próximo programa será executado se, e somente se, o presente não for executado com
 *          sucesso;
 *   Em casos como `c1 || c2 && c3`, `c3` só será executado se ao menos 1 dos operandos de `||` for
 *   verdadeiro.
 *
 * Todos os programas inseridos são executados em processos filhos.
 *
 * Não há nenhum operador built-in além dos supracitados. Isso implica o não funcionamento de
 * comandos como `exit` ou `cd`. Observe que um built-in é um comando de um Shell, e não um
 * executável real.
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