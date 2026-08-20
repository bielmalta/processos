#include <stdio.h>
#include <string.h>

int main() {
    char comando[100];
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
    }
    return 0;
}