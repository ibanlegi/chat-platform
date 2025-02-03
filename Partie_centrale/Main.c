#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

//#define KEY 1234 
#define MAX_UTILISATEURS 100        // Nombre maximum d'utilisateurs
#define MAX_UTILISATEUR_LENGTH 80   // Longueur maximale d'un utilisateur
#define SHARED_MEMORY_SIZE (MAX_UTILISATEURS * MAX_UTILISATEUR_LENGTH) // Taille de la mémoire partagée

// Prototype de fonction
void LancerCommunication(int port);
void TerminerCommunication();
void LancerMemoirePartagee(int cle, int *num_utilisateur);
void TerminerMemoirePartagee();
void gestionnaireSignal(int signum);
void LancerClientRMI(char *nom_serveur);
void TerminerClientRMI();

//Variables globales
int pid_communication; // Variable pour stocker le PID du processus Communication
int pid_clientRMI;
char *adr1; //Adresse mémoire partagée
int status;
int num_utilisateurs = 0; // Nombre d'utilisateurs actuellement stockés

// Fonction pour lancer le processus ClientRMI
void LancerClientRMI(char *nom_serveur) {
    pid_clientRMI = fork();
    if (pid_clientRMI == -1) {
        perror("Erreur lors du fork pour le processus ClientRMI");
        exit(EXIT_FAILURE);
    } else if (pid_clientRMI == 0) {
        // Code exécuté par le processus fils (ClientRMI)
        execlp("java", "java", "ClientRMI", nom_serveur, NULL); // Utilisation de execlp pour exécuter Java avec votre programme
        perror("Erreur lors de l'exécution du ClientRMI");
        exit(EXIT_FAILURE);
    } else {
        // Code exécuté par le processus parent
        printf("Processus ClientRMI lancé avec succès.\n");
    }
}

// Fonction pour terminer proprement le ClientRMI
void TerminerClientRMI() {
    // Envoyer un signal de terminaison au processus ClientRMI
    kill(pid_clientRMI, SIGINT);
    printf("Terminaison du clientRMI.\n");

    // Attendre que le processus ClientRMI se termine
    wait(NULL);
    printf("ClientRMI terminé.\n");
}


// Fonction pour lancer le processus Communication
void LancerCommunication(int port) {
    pid_communication = fork();
    if (pid_communication == -1) {
        perror("Erreur lors du fork pour le processus Communication");
        exit(EXIT_FAILURE);
    } else if (pid_communication == 0) {
        // Code exécuté par le processus fils (Communication)
        char port_str[10];
        sprintf(port_str, "%d", port);
        execl("./Communication", "./Communication", port_str, NULL);
        perror("Erreur lors de l'exécution de Communication");
        exit(EXIT_FAILURE);
    } else {
        // Code exécuté par le processus parent
        printf("Processus Communication lancé avec succès.\n");
    }
}

// Fonction pour terminer proprement la communication
void TerminerCommunication() {
    // Envoyer un signal de terminaison au processus Communication
    kill(pid_communication, SIGINT);
    printf("Terminaison de la communication.\n");

    // Attendre que le processus Communication se termine
    wait(NULL);
    printf("Communication terminée.\n");
}

void LancerMemoirePartagee(int cle, int *num_utilisateur){
    // Création de la mémoire partagée
    if ((status = shmget(cle, SHARED_MEMORY_SIZE + sizeof(int), IPC_CREAT | IPC_EXCL | 0600)) == -1) {
        printf("shm1.shmget: %s\n", strerror(errno));
        exit(1);
    }

    // Attacher la mémoire partagée
    if ((adr1 = (char *)shmat(status, NULL, 0)) == (char *)-1) {
        printf("shm1.shmat: %s\n", strerror(errno));
        exit(1);
    }

    // Initialisation du tableau d'utilisateurs dans la mémoire partagée
    memset(adr1, 0, SHARED_MEMORY_SIZE);

    // Enregistrer le nombre d'utilisateurs en dehors du tableau
    memcpy(adr1 + SHARED_MEMORY_SIZE, num_utilisateur, sizeof(int));


    printf("Mémoire partagée créée et initialisée.\n");
}


void TerminerMemoirePartagee(){
    // Détacher la mémoire partagée
    if (shmdt(adr1) == -1) {
        printf("shm1.shmdt: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Supprimer la mémoire partagée
    if (shmctl(status, IPC_RMID, NULL) == -1) {
        printf("shm1.shmctl: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Mémoire partagée supprimée.\n");
}

// Gestionnaire de signal pour SIGINT (CTRL+C)
void gestionnaireSignal(int signum) {
    printf("\nCRTL+C détecté\n");

    // Terminer proprement la communication
    TerminerCommunication();
    TerminerMemoirePartagee();

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // Vérifier le nombre d'arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <nom_serveur_gestion_comptes> <port_ecoute>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Enregistrer le gestionnaire de signal pour SIGINT (CTRL+C)
    signal(SIGINT, gestionnaireSignal);

    printf("Lancement de la partie centrale : \n");

    //Lancer le processus ClientRMI
    LancerClientRMI(argv[1]);

    // Créer et initialiser la mémoire partagée pour la liste des utilisateurs
    LancerMemoirePartagee(atoi(argv[2]), &num_utilisateurs);

    // Lancer le processus Communication
    LancerCommunication(atoi(argv[2]));

    printf("\n");

    // Attendre que l'utilisateur entre "exit" pour terminer le programme
    char buffer[10];
    printf("Tapez 'exit' pour terminer le programme : ");
    scanf("%s", buffer);
    while (strcmp(buffer, "exit") != 0) {
        printf("Commande inconnue. Tapez 'exit' pour terminer le programme : ");
        scanf("%s", buffer);
    }

    printf("\n\nArrêt de la partie centrale : \n");
    // Terminer proprement la communication
    TerminerCommunication();

    // Supprimer la mémoire partagée
    TerminerMemoirePartagee();

    //Arrêter ClientRMI
    TerminerClientRMI();
    
    return 0;
}