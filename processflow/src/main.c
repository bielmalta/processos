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

int registrar_task(Task tasks[], int total, char comando[]) {
    char copia[200];
    char *nome;
    char *programa;
    char *argumento;

    strcpy(copia, comando);

    strtok(copia, " ");          // ignora a palavra "task"
    nome = strtok(NULL, " ");
    programa = strtok(NULL, " ");

    if (nome == NULL || programa == NULL) {
        printf("Erro: use task <nome> <programa> " "[argumentos...]\n");
        return 0;
    }
    snprintf(tasks[total].nome, sizeof(tasks[total].nome), "%s", nome);

    snprintf(tasks[total].programa, sizeof(tasks[total].programa), "%s",programa);
    tasks[total].total_argumentos = 0;

    argumento = strtok(NULL, " ");
    while (
        argumento != NULL &&
        tasks[total].total_argumentos < MAX_ARGUMENTOS
    ) {
        int posicao = tasks[total].total_argumentos;

        snprintf(tasks[total].argumentos[posicao], sizeof(tasks[total].argumentos[posicao]), "%s", argumento);
        tasks[total].total_argumentos++;
        argumento = strtok(NULL, " ");
    }
    return 1;
}

int achar_task(Task tasks[], int total, char nome[]) {
    for (int i = 0; i < total; i++) {
        if (strcmp(tasks[i].nome, nome) == 0) {
            return i;
        }
    }
    return -1;
}

int run_pipe(Task tasks[], int total, char comando[]) {
    char copia[200];
    char *nome_task;
    int indices[MAX_TASKS];
    int quantidade = 0;

    strcpy(copia, comando + 9);
    nome_task = strtok(copia, " ");
    while (nome_task != NULL && quantidade < MAX_TASKS) {
        int indice = achar_task(tasks, total, nome_task);

        if (indice == -1) {
            printf(
                "Erro: tarefa %s nao encontrada.\n",
                nome_task
            );
            return 0;
        }

        indices[quantidade] = indice;
        quantidade++;

        nome_task = strtok(NULL, " ");
    }
    if (quantidade < 2) {
        printf(
            "Erro: use run pipe "
            "<tarefa1> <tarefa2> ...\n"
        );
        return 0;
    }

    pid_t pids[MAX_TASKS];
    int entrada_anterior = -1;

    for (int i = 0; i < quantidade; i++) {
        int canal[2] = {-1, -1};
        if (i < quantidade - 1) {
            if (pipe(canal) < 0) {
                printf("Erro ao criar pipe.\n");
                if (entrada_anterior != -1) {
                    close(entrada_anterior);
                }
                for (int j = 0; j < i; j++) {
                    waitpid(pids[j], NULL, 0);
                }
                return 0;
            }
        }
        pid_t pid = fork();

        if (pid < 0) {
            printf("Erro ao criar processo.\n");
            if (entrada_anterior != -1) {
                close(entrada_anterior);
            }
            if (canal[0] != -1) {
                close(canal[0]);
                close(canal[1]);
            }
            for (int j = 0; j < i; j++) {
                waitpid(pids[j], NULL, 0);
            }
            return 0;
        }
        if (pid == 0) {
            int indice = indices[i];
            if (entrada_anterior != -1) {
                dup2(entrada_anterior, STDIN_FILENO); //faz a próxima tarefa ler do pipe;
            }
            if (i < quantidade - 1) {
                dup2(canal[1], STDOUT_FILENO); //envia a saída para o pipe;
            }
            if (entrada_anterior != -1) {
                close(entrada_anterior);
            }
            if (canal[0] != -1) {
                close(canal[0]);
                close(canal[1]);
            }
            char *argumentos_exec[MAX_ARGUMENTOS + 2];
            argumentos_exec[0] = tasks[indice].programa;

            for (int j = 0; j < tasks[indice].total_argumentos; j++) {
                argumentos_exec[j + 1] = tasks[indice].argumentos[j];
            }
            argumentos_exec[tasks[indice].total_argumentos + 1] = NULL;
            execv(tasks[indice].programa,argumentos_exec);

            fprintf(stderr,"Erro ao executar a tarefa %s.\n",tasks[indice].nome);
            _exit(1);
        }
        pids[i] = pid;
        if (entrada_anterior != -1) {
            close(entrada_anterior);
        }
        if (i < quantidade - 1) {
            close(canal[1]);
            entrada_anterior = canal[0];
        }
    }
    for (int i = 0; i < quantidade; i++) {
        waitpid(pids[i], NULL, 0);
    }
    return 1;
}

int run_task(Task tasks[], int total, char nome[]) {
    int indice = -1; //O -1 significa que nenhuma tarefa foi encontrada.
    for (int i = 0; i < total; i++) {
        if (strcmp(tasks[i].nome, nome) == 0) {
            indice = i;
            break;
        }
    }
    if (indice == -1) {
        printf("Erro: tarefa %s nao encontrada.\n", nome);
        return 0;
    }
    pid_t pid = fork(); //criei o processo filho
    if (pid < 0) {
        printf("Erro ao criar processo.\n"); 
        return 0;
    }
    if (pid == 0) {
        char *argumentos_exec[MAX_ARGUMENTOS + 2];
        argumentos_exec[0] = tasks[indice].programa;
        for (int j = 0; j < tasks[indice].total_argumentos; j++) {
            argumentos_exec[j + 1] = tasks[indice].argumentos[j]; 
        }
        argumentos_exec[tasks[indice].total_argumentos + 1] = NULL;
        execv(tasks[indice].programa, argumentos_exec); //faz o filho executar o programa.
        printf("Erro: nao foi possivel executar a tarefa.\n");
        fflush(stdout);
        _exit(1); //encerra o processo filho mostrando que deu erro
    }
    waitpid(pid, NULL, 0); //processo pai espera o processo filho terminar
    return 1;
}

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
            if (registrar_task(tasks, total, comando) == 1) {
                printf("Task cadastrada: %s\n", tasks[total].nome);
                total++;
            }

        } else if (strcmp(comando, "run sequential") == 0) {
            printf("Erro: use run sequential " "<tarefa1> <tarefa2> ...\n");

        }else if (strncmp(comando, "run sequential ", 15) == 0) {
            char copia[200];
            char *nome_task;

            strcpy(copia, comando + 15);
            nome_task = strtok(copia, " "); //strtok separa os nomes
            while (nome_task != NULL) {
                if (run_task(tasks, total, nome_task) == 0) {
                    break;
                }
                nome_task = strtok(NULL, " ");
            }
        }
        else if (strcmp(comando, "run parallel") == 0) {
            printf("Erro: use run parallel " "<tarefa1> <tarefa2> ...\n");

        }else if (strncmp(comando, "run parallel ", 13) == 0) {
            char copia[200];
            char *nome_task;
            pid_t pids[MAX_TASKS];
            int total_processos = 0;

            strcpy(copia, comando + 13);
            nome_task = strtok(copia, " ");

            while (nome_task != NULL) {
                int indice = -1;
                for (int i = 0; i < total; i++) {
                    if (strcmp(tasks[i].nome, nome_task) == 0) {
                        indice = i;
                        break;
                    }
                }
                if (indice == -1) {
                    printf("Erro: tarefa %s nao encontrada.\n", nome_task);
                    break;
                }
                if (total_processos == MAX_TASKS) {
                    printf("Erro: limite de processos atingido.\n");
                    break;
                }

                pid_t pid = fork();

                if (pid < 0) {
                    printf("Erro ao criar processo.\n");
                    break;
                }
                if (pid == 0) {
                    char *argumentos_exec[MAX_ARGUMENTOS + 2];
                    argumentos_exec[0] = tasks[indice].programa;
                    for (int j = 0; j < tasks[indice].total_argumentos; j++) {
                        argumentos_exec[j + 1] = tasks[indice].argumentos[j];
                    }

                    argumentos_exec[tasks[indice].total_argumentos + 1] = NULL;

                    execv(tasks[indice].programa, argumentos_exec);
                    printf("Erro: nao foi possivel executar " "a tarefa %s.\n", nome_task);
                    fflush(stdout);
                    _exit(1);
                }

                pids[total_processos] = pid;
                total_processos++;
                nome_task = strtok(NULL, " ");
            }
            for (int i = 0; i < total_processos; i++) {
                waitpid(pids[i], NULL, 0);
            }

        }else if (strcmp(comando, "run pipe") == 0) {
            printf("Erro: use run pipe " "<tarefa1> <tarefa2> ...\n");

        }else if (strncmp(comando, "run pipe ", 9) == 0) {
            run_pipe(tasks, total, comando);

        }else if (strncmp(comando, "run ", 4) == 0) {
            char nome[50];
            if (sscanf(comando, "run %49s", nome) != 1) {
                printf("Erro: use run <nome>\n");
                continue;
            }
            run_task(tasks, total, nome);
        }
    }
    return 0;
}