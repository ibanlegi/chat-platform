#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int fd;
    char buffer[100];
    char ancien_buffer[100] = ""; // Initialiser ancien_buffer avec une chaîne vide
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Erreur lors de l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }
    printf("==== CONVERSATION ====\n");
    while(1){
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer)-1);
        if (bytes_read == -1) {
            perror("Erreur lors de la lecture du fichier");
            exit(EXIT_FAILURE);
        }
        buffer[bytes_read] = '\0';
        if(strcmp(buffer, ancien_buffer) != 0){
            if (strcmp(buffer, "exit") == 0) {
                printf("L'utilisateur s'est déconnecté\n");
                close(fd);
                exit(EXIT_SUCCESS); // Quitter la boucle si "exit" est lu
            }
            fprintf(stdout, "%s\n", buffer);
            strcpy(ancien_buffer, buffer);
        }
    }
    return 0;
}
