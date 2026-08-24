# ProcessFlow

ProcessFlow é um gerenciador de processos desenvolvido em C para Linux. O programa utiliza `fork()`, `execv()`, `waitpid()`, `pipe()` e `dup2()` para executar e controlar tarefas.

## Compilação

Na raiz do projeto:

```bash
make
```

Para limpar os arquivos compilados:

```bash
make clean
```

## Execução

Modo interativo:

```bash
./processflow/processflow
```

Workflow por arquivo:

```bash
./processflow/processflow teste.pf
```

## Comandos

```text
task <nome> <programa> [argumentos...]
run <task>
run sequential <task1> <task2> ...
run parallel <task1> <task2> ...
run pipe <task1> <task2> ...
input <task> <arquivo>
output <task> <arquivo>
append <task> <arquivo>
workdir <diretorio>
start <task>
jobs
wait <id>
exit
```

## Exemplo

```text
task listar /bin/ls
task ordenar /usr/bin/sort
task contar /usr/bin/wc -l

run listar
run pipe listar ordenar contar
exit
```

## Requisitos

- Linux ou WSL;
- GCC;
- Make.