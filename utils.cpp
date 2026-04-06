#include "utils.h"
#include "parser.h"
#include <iostream>
#include <print>
#include <stdlib.h>
#include <string>

using namespace std;

void debug_loop()
{
  while (true)
  {
    auto line = string();
    getline(cin, line);

    auto tokens_result = Lexer().tokenize(line);

    if (!tokens_result)
    {
      println("{}", tokens_result.error().what());
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

    auto parser_result = Parser::parse_commands(line);

    if (!parser_result)
    {
      println("{}", parser_result.error().what());
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