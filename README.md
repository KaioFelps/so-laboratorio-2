# Mini Shell

## Executando
Executar o programa com a flag `-d` roda o modo de debug — dado um comando, será exebido o
resultado da análise léxica e a fila de comandos resultante.

Em modo normal, todo comando digitado será transformado numa fila de comandos, e cada um
será executado de acordo com os metadados do comando. Comandos filhos de outros comandos
têm preferência sobre os próximos comandos da fila de execução. Essa é uma abordagem híbrida
entre uma fila simples e uma AST completa.

## Estrutura
O programa lida com três tipos de dados principais:
- Argumentos - são strings que formam um comando;
- Separadores - são entradas que separam um comando do outro (`&` ou `;`):
    - `&`: o programa anterior é executado em plano de fundo;
    - `;`: o próximo programa é executado independentemente do sucesso ou fracasso do anterior.
- Operadores - são uma variação dos separadores: encerram um comando, mas ao invés de iniciar um
  novo comando na fila de execução, o próximo comando é salvo na estrutura do comando atual
  dada a operação lógica utilizada:
    - `&&`: o próximo programa será executado se, e somente se, o presente for executado com
            sucesso;
    - `||`: o próximo programa será executado se, e somente se, o presente não for executado com
            sucesso;
  Em casos como `c1 || c2 && c3`, `c3` só será executado se ao menos 1 dos operandos de `||` for
  verdadeiro.

Todos os programas inseridos são executados em processos filhos.

## Limitações
Não há nenhum operador built-in além dos supracitados. Isso implica o não funcionamento de
comandos como `exit` ou `cd`. Observe que um built-in é um comando de um Shell, e não um
executável real.

### Subshells
Em um shell totalmente funcional, comandos como `(c1 && c2)&`, agrupados por parênteses, são
interpretados como um subshell. Isso é, o processo filho criado executa, na verdade, o próprio
executável do shell em modo standalone — executa somente esse único comando e depois encerra.

Para as finalidades deste laboratório, o agrupamento por parênteses serve apenas para viabilizar
comandos como `(c1 &) && c2`. Dessa forma, o comando `(c1 && c2)&` é equivalente a
`(c1 &) && (c2 &)`.
