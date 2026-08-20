#include <stdio.h>
#include <string.h>

#define MAX_TASK 20

typedef struct {
    char nome[50];
    char programa[100];
} Task;

int main() {
    Task tasks[MAX_TASK];
    int total = 0;
    char comando[200];
    while (1) {
        printf("processflow> ");
        fflush(stdout);
        fgets(comando, sizeof(comando), stdin);
        comando[strcspn(comando, "\n")] = '\0';
        if (strcmp(comando, "exit") == 0) {
            break;
        }
        if (strncmp(comando, "task ", 5) == 0) {
            char nome[50];
            char programa[100];
            if (sscanf(comando, "task %49s %99s", nome, programa) != 2) { //Ler uma palavra de até 49 caracteres. Tem que deixar 1 posição para o \0
                printf("Erro: use task <nome> <programa>\n");
                continue;
            }
            strcpy(tasks[total].nome, nome);
            strcpy(tasks[total].programa, programa);
            total++;
            printf("Tarefa cadastrada: %s\n", nome);
        }
    }
    return 0;
}