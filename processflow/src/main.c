#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TASKS 20

typedef struct {
    char nome[50];
    char programa[100];
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
            if (sscanf(comando, "task %49s %99s", nome, programa) != 2) {
                printf("Erro: use task <nome> <programa>\n");
                continue;
            }
            strcpy(tasks[total].nome, nome);
            strcpy(tasks[total].programa, programa);
            total++;
            printf("Task cadastrada: %s\n", nome);
        }
        else if (strncmp(comando, "run ", 4) == 0) {
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
                        execl(tasks[i].programa,
                              tasks[i].nome,
                              NULL);

                        printf("Erro: nao foi possivel executar a tarefa.\n");
                        return 1;
                    }
                    waitpid(pid, NULL, 0);
                    break;
                }
            }
            if (!encontrada) { //não encontrei a task
                printf("Erro: tarefa nao encontrada.\n");
            }
        }
    }
    return 0;
}