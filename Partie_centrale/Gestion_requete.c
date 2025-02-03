#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h> // Pour shmget
#include <sys/shm.h> // Pour shmat et shmdt
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define PORT_SERVEUR 7777
#define MAX_BUFFER_SIZE 2024
#define TIMEOUT 3
#define MAX_UTILISATEURS 100                                      // Nombre maximum de  l'utilisateurs
#define MAX_UTILISATEUR_LONGUEUR 80                                 // Longueur maximale d'un utilisateur
#define TAILLE_MEMOIRE_PARTAGEE (MAX_UTILISATEURS * MAX_UTILISATEUR_LONGUEUR) // Taille de la mémoire partagée

//Déclarations de fonctions
void Gestionnaire_timeout(int signum);
int ConfigurerSocket();
void EnvoyerMessage(int socketClient, const char *message);
void RecevoirReponse(int socketClient, char *buffer);
char *ObtenirMemoirePartagee(int cle);
void AfficherUtilisateurs(char *adr1, int *num_utilisateurs);
char *AfficherUtilisateursPourEnvoi(char *adr1, int *num_utilisateurs);
int AjouterUtilisateur(char *adr1, int *num_utilisateurs, char * pseudo);
int SupprimerUtilisateur(char *adr1, int *num_utilisateurs, char * pseudo);
void DetachementMemoirePartagee(char *adr1, int *num_utilisateurs);

typedef struct {
    char pseudo[MAX_BUFFER_SIZE]; // Pseudo de l'utilisateur
    int socket; // Socket associée à l'utilisateur (pour la communication)
} UtilisateurActif;

#define MAX_UTILISATEURS_ACTIFS 10
UtilisateurActif utilisateurs_actifs[MAX_UTILISATEURS_ACTIFS];
int nb_utilisateurs_actifs = 0;


// Fonction de gestion du timeout
void Gestionnaire_timeout(int signum) {
    exit(2);
}

// Configuration du socket client UDP
int ConfigurerSocket() {
    int socketClient;
    if ((socketClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }
    return socketClient;
}

// Envoi du message au serveur
void EnvoyerMessage(int socketClient, const char *message) {
    struct sockaddr_in serveurAddr;
    memset((char *)&serveurAddr, 0, sizeof(serveurAddr));
    serveurAddr.sin_family = AF_INET;
    serveurAddr.sin_port = htons(PORT_SERVEUR);
    serveurAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Adresse locale (127.0.0.1)

    if (sendto(socketClient, message, strlen(message), 0, (struct sockaddr *)&serveurAddr, sizeof(serveurAddr)) == -1) {
        perror("Erreur lors de l'envoi du message");
        close(socketClient); 
        exit(EXIT_FAILURE);
    }
}

// Réception de la réponse du serveur
void RecevoirReponse(int socketClient, char *buffer) {
    struct sockaddr_in serveurAddr;
    socklen_t addrLength = sizeof(serveurAddr);
    int bytesReceived = recvfrom(socketClient, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&serveurAddr, &addrLength);
    if (bytesReceived == -1) {
        perror("Erreur lors de la réception de la réponse du serveur");
        close(socketClient);
        exit(EXIT_FAILURE);
    }
    buffer[bytesReceived] = '\0';
}

// Fonction pour récupérer la mémoire partagée
char *ObtenirMemoirePartagee(int cle) {
    int status;
    char *adr1;

    // Récupération de la mémoire partagée
    if ((status = shmget(cle, TAILLE_MEMOIRE_PARTAGEE, 0)) == -1) {
        printf("shm2.shmget: %s\n", strerror(errno));
        exit(1);
    }

    // Attacher la mémoire partagée
    if ((adr1 = (char *)shmat(status, NULL, 0)) == (char *)-1) {
        printf("shm2.shmat: %s\n", strerror(errno));
        exit(1);
    }

    return adr1;
}

// Fonction pour afficher la liste des utilisateurs
void AfficherUtilisateurs(char *adr1, int *num_utilisateurs) {
    printf("Liste des utilisateurs :\n\n");

    // Parcourir chaque utilisateur dans la mémoire partagée
    for (int i = 0; i < *num_utilisateurs; i++) {
        char *utilisateur = adr1 + (i * MAX_UTILISATEUR_LONGUEUR);
        if (strlen(utilisateur) > 0) {
            printf("%s\n", utilisateur);
        }
    }
}

// Fonction pour afficher la liste des utilisateurs au format d'envoi
char *AfficherUtilisateursPourEnvoi(char *adr1, int *num_utilisateurs) {
    static char liste_utilisateurs[MAX_UTILISATEURS * (MAX_UTILISATEUR_LONGUEUR + 1)] = ""; // +1 pour le caractère de séparation '_'

    // Parcourir chaque utilisateur dans la mémoire partagée
    for (int i = 0; i < *num_utilisateurs; i++) {
        char *utilisateur = adr1 + (i * MAX_UTILISATEUR_LONGUEUR);
        if (strlen(utilisateur) > 0) {
            strncat(liste_utilisateurs, utilisateur, MAX_UTILISATEUR_LONGUEUR); // Concatène le utilisateur
            if (i < *num_utilisateurs - 1) {
                strncat(liste_utilisateurs, "_", sizeof("_"));    // Ajoute le séparateur '_'
            }
        }
    }

    // Retourner la liste des utilisateurs
    return liste_utilisateurs;
}



// Fonction pour ajouter un utilisateur à la mémoire partagée
int AjouterUtilisateur(char *adr1, int *num_utilisateurs, char *pseudo) {
    // Vérifier si le tableau d'utilisateurs est plein
    if (*num_utilisateurs >= MAX_UTILISATEURS) {
        printf("Le tableau d'utilisateurs est plein.\n");
        return 1;
    }

    // Copier l'utilisateur dans le tableau d'utilisateurs
    strncpy(adr1 + ((*num_utilisateurs) * MAX_UTILISATEUR_LONGUEUR), pseudo, MAX_UTILISATEUR_LONGUEUR);
    (*num_utilisateurs)++;

    printf("Utilisateur ajouté à la mémoire partagée : %s\n", pseudo);
    
    return 0;
}

// Fonction pour supprimer un utilisateur de la mémoire partagée
int SupprimerUtilisateur(char *adr1, int *num_utilisateurs, char *pseudo) {
    // Rechercher l'utilisateur à supprimer dans le tableau d'utilisateurs
    int found = 0;
    for (int i = 0; i < *num_utilisateurs; i++) {
        if (strcmp(adr1 + (i * MAX_UTILISATEUR_LONGUEUR), pseudo) == 0) {
            // Déplacer les utilisateurs suivants pour remplir l'emplacement supprimé
            for (int j = i; j < *num_utilisateurs - 1; j++) {
                strncpy(adr1 + (j * MAX_UTILISATEUR_LONGUEUR), adr1 + ((j + 1) * MAX_UTILISATEUR_LONGUEUR), MAX_UTILISATEUR_LONGUEUR);
            }
            (*num_utilisateurs)--;
            printf("Utilisateur supprimé de la mémoire partagée : %s\n", pseudo);
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("L'utilisateur n'existe pas dans la mémoire partagée.\n");
    } else {
        // Effacer la dernière occurrence de l'utilisateur pour éviter les doublons
        memset(adr1 + (*num_utilisateurs * MAX_UTILISATEUR_LONGUEUR), 0, MAX_UTILISATEUR_LONGUEUR);
    }
    return found ? 0 : 1; // Retourner 0 si l'utilisateur a été trouvé et supprimé, sinon 1
}






// Fonction pour détacher la mémoire partagée et mettre à jour le nombre d'utilisateurs
void DetachementMemoirePartagee(char *adr1, int *num_utilisateurs) {
    // Copier le nouveau nombre d'utilisateurs dans la mémoire partagée
    memcpy(adr1 + TAILLE_MEMOIRE_PARTAGEE, num_utilisateurs, sizeof(int));
    
    // Détacher la mémoire partagée
    if (shmdt(adr1) == -1) {
        printf("shm2.shmdt: %s\n", strerror(errno));
        exit(1);
    }
}


// Fonction principale
int main(int argc, char *argv[]) {
    if (argc != 3){
        fprintf(stderr, "Usage: C %s <requete> <port_ecoute>\n", argv[0]);
        exit(1);
    }

    //Variables globales
    int pid_communication; // Variable pour stocker le PID du processus Communication
    char *adr1; //Adresse mémoire partagée
    int status;

    adr1 = ObtenirMemoirePartagee(atoi(argv[2])); // Obtention de la mémoire partagée
    
    int num_utilisateurs;
    // Récupérer le nombre d'utilisateurs actuellement connectés à partir de la mémoire partagée
    memcpy(&num_utilisateurs, adr1 + TAILLE_MEMOIRE_PARTAGEE, sizeof(int));

    // Définir le gestionnaire de signal pour SIGALRM
    signal(SIGALRM, Gestionnaire_timeout);

    // Configuration du socket
    int socketClient = ConfigurerSocket();

    // Analyse de la réponse du serveur
    char temp_message[MAX_BUFFER_SIZE];
    strcpy(temp_message, argv[1]);
    char *token;
    token = strtok(temp_message, "_");

    // Préparation du message
    char message[MAX_BUFFER_SIZE];
    sprintf(message, "%s", argv[1]);
    printf("\tUsage C %s Message [ %s ]\n",argv[0], message);

     char buffer[MAX_BUFFER_SIZE];

    // Vérifier si le message contient une requête spéciale
    if (strstr(message, "QUITTER") != NULL) {

        printf("\tRequête contient QUITTER. Terminaison du programme client.\n");
        
        token = strtok(NULL, "_"); //Obtenir le pseudo
        int etatMP = SupprimerUtilisateur(adr1, &num_utilisateurs, token); // Supprimer l'utilisateur de la mémoire partagée
        DetachementMemoirePartagee(adr1, &num_utilisateurs); // Détacher la mémoire partagée après utilisation
        close(socketClient);

        return 0;

    } else if(strstr(message, "LISTEUTILISATEUR") != NULL){
        
        printf("AFFICHAGE LISTE : \n");
        AfficherUtilisateurs(adr1, &num_utilisateurs);

        printf("\n\nAFFICHAGE LISTE : %s\n", AfficherUtilisateursPourEnvoi(adr1, &num_utilisateurs));
        DetachementMemoirePartagee(adr1, &num_utilisateurs); // Détacher la mémoire partagée après utilisation
        close(socketClient);

        return 3;

    } else if(strstr(message, "MESSAGE") != NULL){

        printf("\tRequête MESSAGE. Non implémentée.\n");
        DetachementMemoirePartagee(adr1, &num_utilisateurs); // Détacher la mémoire partagée avant de quitter
        close(socketClient);

        return 4;

    } else {
        // Définir la minuterie pour le timeout
        alarm(TIMEOUT);
        // Envoi du message au serveur
        EnvoyerMessage(socketClient, message);

        // Réception de la réponse du serveur
        RecevoirReponse(socketClient, buffer);

        // Réinitialiser la minuterie du timeout
        alarm(0);
    }

    // Afficher la réponse du serveur
    printf("\tRéponse du serveur : %s\n", buffer);

    //Traitement de la réponse serveur
    printf("\tUsage C %s token [ %s ]\n",argv[0], token);
    if (token != NULL && strcmp(token, "CONNEXION") == 0 && strcmp(buffer, "CONNEXION_VALIDE") == 0) {

        token = strtok(NULL, "_"); //Obtenir le pseudo
        int etatMP = AjouterUtilisateur(adr1, &num_utilisateurs, token); // Ajouter l'utilisateur à la mémoire partagée
        DetachementMemoirePartagee(adr1, &num_utilisateurs); // Détacher la mémoire partagée après utilisation

        return etatMP;

    } else if (token != NULL && strcmp(token, "CREERCOMPTE") == 0 && strcmp(buffer, "CREERCOMPTE_VALIDE") == 0){

        DetachementMemoirePartagee(adr1, &num_utilisateurs);

        return 0;

    } else if (token != NULL && strcmp(token, "DECONNEXION") == 0){

        token = strtok(NULL, "_"); //Obtenir le pseudo
        int etatMP = SupprimerUtilisateur(adr1, &num_utilisateurs, token); // Supprimer l'utilisateur de la mémoire partagée
        DetachementMemoirePartagee(adr1, &num_utilisateurs); // Détacher la mémoire partagée après utilisation

        return etatMP;

    } else if (token != NULL && strcmp(token, "SUPPRIMERCOMPTE") == 0 && strcmp(buffer, "SUPPRIMERCOMPTE_VALIDE") == 0){

        token = strtok(NULL, "_"); //Obtenir le pseudo
        int etatMP = SupprimerUtilisateur(adr1, &num_utilisateurs, token); // Supprimer l'utilisateur de la mémoire partagée
        DetachementMemoirePartagee(adr1, &num_utilisateurs); // Détacher la mémoire partagée après utilisation

        return etatMP;

    } else {

        return 1;
    }

}
