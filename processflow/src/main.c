#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TASKS 20
#define MAX_ARGUMENTOS 10

typedef struct {
    char nome[50];
    char programa[100];
    char argumentos[MAX_ARGUMENTOS][100];
    int total_argumentos;
} Task;

int main() {
    Task tasks[MAX_TASKS];
    int total = 0;
    char comando[200];

    while (1) {
        printf("processflow> ");
        fflush(stdout);
        if (fgets(comando, sizeof(comando), stdin) == NULL) {
            break;
        }
        comando[strcspn(comando, "\n")] = '\0';
        if (strcmp(comando, "exit") == 0) {
            break;
        }
        if (strncmp(comando, "task ", 5) == 0) {
            char nome[50];
            char programa[100];
            char texto_argumentos[200] = "";
            char *argumento;
            int quantidade = sscanf(
                comando,
                "task %49s %99s %199[^\n]",
                nome,
                programa,
                texto_argumentos
            );
            if (quantidade < 2) {
                printf(
                    "Erro: use task <nome> <programa> [argumentos...]\n"
                );
                continue;
            }
            strcpy(tasks[total].nome, nome);
            strcpy(tasks[total].programa, programa);
            tasks[total].total_argumentos = 0;
            if (quantidade == 3) {
                argumento = strtok(texto_argumentos, " ");
                while (argumento != NULL && tasks[total].total_argumentos < MAX_ARGUMENTOS) {
                    int posicao = tasks[total].total_argumentos;
                    snprintf(tasks[total].argumentos[posicao], sizeof(tasks[total].argumentos[posicao]), "%s",argumento);
                    tasks[total].total_argumentos++;
                    argumento = strtok(NULL, " ");
                }
            }
            total++;
            printf("Task cadastrada: %s\n", nome);

        }else if (strncmp(comando, "run ", 4) == 0) {
            char nome[50];
            int encontrada = 0;
            sscanf(comando, "run %49s", nome);
            for (int i = 0; i < total; i++) {
                if (strcmp(tasks[i].nome, nome) == 0) {
                    encontrada = 1;
                    pid_t pid = fork();
                    if (pid < 0) {
                        printf("Erro ao criar processo.\n");
                        break;
                    }
                    if (pid == 0) {
                        char *argumentos_exec[MAX_ARGUMENTOS + 2];
                        argumentos_exec[0] = tasks[i].programa;
                        for (int j = 0; j < tasks[i].total_argumentos; j++) {
                            argumentos_exec[j + 1] = tasks[i].argumentos[j];
                        }
                        argumentos_exec[tasks[i].total_argumentos + 1] = NULL;
                        execv(tasks[i].programa, argumentos_exec);
                        printf("Erro: nao foi possivel executar a tarefa.\n");
                        return 1;
                    }
                    waitpid(pid, NULL, 0);
                    break;
                }
            }
            if (!encontrada) {
                printf("Erro: tarefa nao encontrada.\n");
            }
        }
    }
    return 0;
}