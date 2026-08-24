#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_TASKS 20
#define MAX_ARGUMENTOS 10
#define MAX_JOBS 20
char workdir_path[200] = "";

typedef struct {
    char nome[50];
    char programa[100];
    char argumentos[MAX_ARGUMENTOS][100];
    int total_argumentos;
    char input_file[200];
    char output_file[200];
    int append_mode;
} Task;

typedef struct Job{
    int id;
    pid_t pid;
    char nome_task[50];
    int ativo;
}Job;

Job jobs[MAX_JOBS];
int total_jobs = 0;
int proximo_job_id = 1;

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
    tasks[total].input_file[0] = '\0';
    tasks[total].output_file[0] = '\0';
    tasks[total].append_mode = 0;

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

int armazenar_arquivos(Task tasks[], int total, char comando[], char tipo[]) {
    char nome[50];
    char arquivo[200];
    int quantidade = sscanf(comando,"%*s %49s %199s",nome,arquivo);

    if (quantidade != 2) {
        printf("Erro: use %s <tarefa> <arquivo>\n", tipo);
        return 0;
    }
    int indice = achar_task(tasks, total, nome);
    if (indice == -1) {
        printf("Erro: tarefa %s nao encontrada.\n", nome);
        return 0;
    }

    if (strcmp(tipo, "input") == 0) {
        snprintf(tasks[indice].input_file,sizeof(tasks[indice].input_file), "%s", arquivo);
    }else {
        snprintf(tasks[indice].output_file, sizeof(tasks[indice].output_file),"%s", arquivo);
        if (strcmp(tipo, "append") == 0) {
            tasks[indice].append_mode = 1;
        }
        else {
            tasks[indice].append_mode = 0;
        }
    }
    printf("%s configurado para a tarefa %s.\n",tipo,nome);
    return 1;
}

int configure_workdir(char comando[]) {
    char caminho[200];
    struct stat informacoes;

    int quantidade = sscanf(comando, "workdir %199[^\n]", caminho);

    if (quantidade != 1) {
        printf("Erro: use workdir <diretorio>\n");
        return 0;
    }

    if (stat(caminho, &informacoes) != 0 || !S_ISDIR(informacoes.st_mode)) {
        printf("Erro: diretorio nao encontrado.\n");
        return 0;
    }

    snprintf(workdir_path, sizeof(workdir_path), "%s", caminho);

    printf("Diretorio de trabalho configurado: %s\n", caminho);
    return 1;
}


int aplicar_workdir() {
    if (workdir_path[0] == '\0') {
        return 1;
    }

    if (chdir(workdir_path) != 0) {
        perror("Erro ao acessar diretorio de trabalho");
        return 0;
    }
    return 1;
}

int redirecionamento(Task tasks[], int indice) {
    int arquivo;
    if (tasks[indice].input_file[0] != '\0') {
        arquivo = open(tasks[indice].input_file, O_RDONLY);
        if (arquivo < 0) {
            perror("Erro ao abrir arquivo de entrada");
            return 0;
        }
        if (dup2(arquivo, STDIN_FILENO) < 0) {
            perror("Erro ao redirecionar entrada");
            close(arquivo);
            return 0;
        }
        close(arquivo);
    }
    if (tasks[indice].output_file[0] != '\0') {
        int opcoes = O_WRONLY | O_CREAT;
        if (tasks[indice].append_mode == 1) {
            opcoes = opcoes | O_APPEND;
        }else {
            opcoes = opcoes | O_TRUNC;
        }
        arquivo = open(tasks[indice].output_file,opcoes,0644);
        if (arquivo < 0) {
            perror("Erro ao abrir arquivo de saida");
            return 0;
        }
        if (dup2(arquivo, STDOUT_FILENO) < 0) {
            perror("Erro ao redirecionar saida");
            close(arquivo);
            return 0;
        }
        close(arquivo);
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
        if (aplicar_workdir() == 0) {
            _exit(1);
        }
        if (redirecionamento(tasks, indice) == 0) {
            _exit(1);
        }

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

int start_task(Task tasks[], int total, char nome[]) {
    int indice = achar_task(tasks, total, nome);

    if (indice == -1) {
        printf("Erro: tarefa %s nao encontrada.\n", nome);
        return 0;
    }

    if (total_jobs == MAX_JOBS) {
        printf("Erro: limite de jobs atingido.\n");
        return 0;
    }

    pid_t pid = fork();

    if (pid < 0) {
        printf("Erro ao criar processo.\n");
        return 0;
    }

    /* Processo filho */
    if (pid == 0) {
        if (aplicar_workdir() == 0) {
            _exit(1);
        }
        if (redirecionamento(tasks, indice) == 0) {
            _exit(1);
        }
        char *argumentos_exec[MAX_ARGUMENTOS + 2];
        argumentos_exec[0] = tasks[indice].programa;

        for (int i = 0;i < tasks[indice].total_argumentos;i++) {
            argumentos_exec[i + 1] = tasks[indice].argumentos[i];
        }

        argumentos_exec[tasks[indice].total_argumentos + 1] = NULL;
        execv(tasks[indice].programa, argumentos_exec);
        perror("Erro no execv");
        _exit(1);
    }

    jobs[total_jobs].id = proximo_job_id;
    jobs[total_jobs].pid = pid;
    jobs[total_jobs].ativo = 1;

    snprintf(jobs[total_jobs].nome_task,sizeof(jobs[total_jobs].nome_task),"%s", nome);
    printf("[Job %d] PID %d - tarefa %s iniciada\n", jobs[total_jobs].id, (int)pid, jobs[total_jobs].nome_task);

    total_jobs++;
    proximo_job_id++;
    return 1;
}

void listar_jobs() {
    if (total_jobs == 0) {
        printf("Nenhum job registrado.\n");
        return;
    }
    for (int i = 0; i < total_jobs; i++) {
        if (jobs[i].ativo == 1) {
            pid_t resultado = waitpid(jobs[i].pid, NULL, WNOHANG);
            if (resultado == jobs[i].pid) {
                jobs[i].ativo = 0;
            }
        }

        if (jobs[i].ativo == 1) {
            printf("[Job %d] PID %d - %s - executando\n",jobs[i].id, (int)jobs[i].pid,jobs[i].nome_task);
        }else {
            printf("[Job %d] PID %d - %s - finalizado\n",jobs[i].id,(int)jobs[i].pid,jobs[i].nome_task);
        }
    }
}

int esperar_job(int id) {
    int indice = -1;
    for (int i = 0; i < total_jobs; i++) {
        if (jobs[i].id == id) {
            indice = i;
            break;
        }
    }
    if (indice == -1) {
        printf("Erro: job %d nao encontrado.\n", id);
        return 0;
    }
    if (jobs[indice].ativo == 0) {
        printf("Job %d ja foi finalizado.\n", id);
        return 1;
    }
    printf("Aguardando job %d...\n", id);
    pid_t resultado = waitpid(jobs[indice].pid,NULL,0);
    if (resultado < 0) {
        perror("Erro ao esperar job");
        return 0;
    }
    jobs[indice].ativo = 0;
    printf("Job %d finalizado.\n", id);
    return 1;
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
        printf("Erro: use run pipe " "<tarefa1> <tarefa2> ...\n");
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
            if (aplicar_workdir() == 0){
                _exit(1);
            }

            if (redirecionamento(tasks, indice) == 0) {
                _exit(1);
            }
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

int main(int argc, char *argv[]) {
    Task tasks[MAX_TASKS];
    int total = 0;
    char comando[200];

    FILE *entrada = stdin;
    int modo_arquivo = 0;
    if (argc > 2) {
        printf("Erro: use ./processflow [arquivo.pf]\n");
        return 1;
    }
    if (argc == 2) {
        entrada = fopen(argv[1], "r");
        if (entrada == NULL) {
            perror("Erro ao abrir workflow");
            return 1;
        }
        modo_arquivo = 1;
    }

    while (1) {
        if (modo_arquivo == 0) {
            printf("processflow> ");
            fflush(stdout);
        }
        if (fgets(comando, sizeof(comando), entrada) == NULL) {
            break;
        }
        comando[strcspn(comando, "\r\n")] = '\0';
        if (comando[0] == '\0') {
            continue;
        }
        if (modo_arquivo == 1) {
            printf("processflow> %s\n", comando);
        }
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
                    if (aplicar_workdir() == 0) {
                        _exit(1);
                    }
                    if (redirecionamento(tasks, indice) == 0) {
                        _exit(1);
                    }
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
        
        }else if (strcmp(comando, "workdir") == 0 || strncmp(comando, "workdir ", 8) == 0) {
            configure_workdir(comando);
        }else if (strcmp(comando, "input") == 0 || strncmp(comando, "input ", 6) == 0) {
            armazenar_arquivos(tasks, total, comando, "input");

        }else if (strcmp(comando, "output") == 0 || strncmp(comando, "output ", 7) == 0 ){
            armazenar_arquivos(tasks, total, comando, "output");

        }else if (strcmp(comando, "append") == 0 || strncmp(comando, "append ", 7) == 0) {
            armazenar_arquivos(tasks, total, comando, "append");

        }else if (strcmp(comando, "start") == 0) {
             printf("Erro: use start <tarefa>\n");
        }else if (strncmp(comando, "start ", 6) == 0) {
            char nome[50];
            if (sscanf(comando, "start %49s", nome) != 1) {
                printf("Erro: use start <tarefa>\n");
                continue;
            }
            start_task(tasks, total, nome);

        }else if (strcmp(comando, "jobs") == 0) {
            listar_jobs();

        }else if (strcmp(comando, "wait") == 0) {
            printf("Erro: use wait <id>\n");
        }else if (strncmp(comando, "wait ", 5) == 0) {
            int id;
            if (sscanf(comando, "wait %d", &id) != 1) {
                printf("Erro: use wait <id>\n");
                continue;
            }
            esperar_job(id);

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
    if (entrada != stdin) {
        fclose(entrada);
    }
    return 0;
}