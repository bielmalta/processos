#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

    pid_t pid = fork(); //criei o processo filho 

    if (pid < 0) {
        printf("Erro ao criar processo.\n");
        return 1;
    }

    if (pid == 0) { //estou dentro do processo filho
        printf("Processo filho executando ls.\n"); 

        execl("/bin/ls", "ls", NULL); //filho executando o processo

        printf("Erro ao executar ls.\n");
        return 1;
    }

    printf("Processo pai esperando o filho.\n");

    wait(NULL); //processo pai esperando o processo filho terminar

    printf("O filho terminou. Pai encerrando.\n");

    return 0;
}