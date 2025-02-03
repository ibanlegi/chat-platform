#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/ipc.h>
#include <sys/shm.h>


#define TAILLEBUF 2000
#define MAX_CLIENTS 10

#define MAX_UTILISATEURS 100                                      // Nombre maximum de  l'utilisateurs
#define MAX_UTILISATEUR_LONGUEUR 80                                 // Longueur maximale d'un utilisateur
#define TAILLE_MEMOIRE_PARTAGEE (MAX_UTILISATEURS * MAX_UTILISATEUR_LONGUEUR) // Taille de la mémoire partagée

//Déclarations des fonctions
void *ProcessusRequetes(void *arg);
void *ClientThread(void *arg);
void DemarrerServeur(int numero_port);
void AjouterClient(int client_socket, struct sockaddr_in client_addr);
void SupprimerClient(int client_socket);
char *ObtenirMemoirePartagee(int cle);
void DetachementMemoirePartagee(char *adr1, int *num_utilisateurs);
char *AfficherUtilisateursPourEnvoi(char *adr1, int *num_utilisateurs);
char *RetourListe(int cle);

//Déclarations des structures

// Structure pour représenter une requête client
typedef struct {
    char message[TAILLEBUF];
    int client_socket;
} ClientRequete;

//Structure représentant la file de message
typedef struct {
    ClientRequete tab_message[MAX_CLIENTS];
    int debut;
    int fin;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} FileMessage;

// Structure pour stocker les informations d'un client
typedef struct {
    int socket;
    struct sockaddr_in adresse;
} ClientInfo;

//Structure permettant le passage en parametre des threads
typedef struct {
    int client_socket;
    FileMessage *file;
} ClientThreadArgs; 

// Tableau pour stocker les informations des clients connectés
ClientInfo clients[MAX_CLIENTS];
int nombre_clients = 0;
pthread_mutex_t mutex_clients = PTHREAD_MUTEX_INITIALIZER;

int port;

char liste_utilisateurs[TAILLEBUF] = "";

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

void DetachementMemoirePartagee(char *adr1, int *num_utilisateurs) {
    // Copier le nouveau nombre d'utilisateurs dans la mémoire partagée
    memcpy(adr1 + TAILLE_MEMOIRE_PARTAGEE, num_utilisateurs, sizeof(int));
    
    // Détacher la mémoire partagée
    if (shmdt(adr1) == -1) {
        printf("shm2.shmdt: %s\n", strerror(errno));
        exit(1);
    }
}

char *AfficherUtilisateursPourEnvoi(char *adr1, int *num_utilisateurs) {
    //static char liste_utilisateurs[MAX_UTILISATEURS * (MAX_UTILISATEUR_LONGUEUR + 1)] = ""; // +1 pour le caractère de séparation '_'

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

// Fonction récupérer liste de la mémoire partagée
char *RetourListe(int cle) {
    char *adr1; // Adresse mémoire partagée
    int status;

    // Obtention de la mémoire partagée
    adr1 = ObtenirMemoirePartagee(cle);

    int num_utilisateurs;
    // Récupérer le nombre d'utilisateurs actuellement connectés à partir de la mémoire partagée
    memcpy(&num_utilisateurs, adr1 + TAILLE_MEMOIRE_PARTAGEE, sizeof(int));

    // Effacer la liste précédente avant de la mettre à jour
    memset(liste_utilisateurs, 0, sizeof(liste_utilisateurs));

    // Appel de la fonction AfficherUtilisateursPourEnvoi pour obtenir la liste des utilisateurs
    AfficherUtilisateursPourEnvoi(adr1, &num_utilisateurs);

    // Détacher la mémoire partagée
    DetachementMemoirePartagee(adr1, &num_utilisateurs);

    // Retourner la liste des utilisateurs
    return liste_utilisateurs;
}

// Fonction pour enregistrer une nouvelle connexion client
void AjouterClient(int client_socket, struct sockaddr_in client_addr) {
    pthread_mutex_lock(&mutex_clients);
    if (nombre_clients < MAX_CLIENTS) {
        clients[nombre_clients].socket = client_socket;
        clients[nombre_clients].adresse = client_addr;
        printf("Nouveau client connecté depuis l'adresse IP : %s\n", inet_ntoa(client_addr.sin_addr)); // Afficher l'adresse IP du client
        nombre_clients++;
    }
    pthread_mutex_unlock(&mutex_clients);
}

// Fonction pour supprimer une connexion client
void SupprimerClient(int client_socket) {
    pthread_mutex_lock(&mutex_clients);
    for (int i = 0; i < nombre_clients; i++) {
        if (clients[i].socket == client_socket) {
            // Décaler les clients suivants vers la gauche pour remplir le trou
            printf("Client déconnecté depuis l'adresse IP : %s\n", inet_ntoa(clients[i].adresse.sin_addr)); // Afficher l'adresse IP du client déconnecté
            for (int j = i; j < nombre_clients - 1; j++) {
                clients[j] = clients[j + 1];
            }
            nombre_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_clients);
}

// Fonction pour traiter les requêtes clients
void *ProcessusRequetes(void *arg) {
    FileMessage *file = (FileMessage *)arg; // Convertir l'argument en pointeur vers la structure FileMessage

    while (1) {
        pthread_mutex_lock(&(file->mutex));
        // Attendre qu'une requête soit disponible dans la file de messages
        while (file->debut == file->fin) {
            pthread_cond_wait(&(file->cond), &(file->mutex));
        }
        // Récupérer la requête de la file de messages
        ClientRequete requete = file->tab_message[file->debut];
        file->debut = (file->debut + 1) % MAX_CLIENTS;
        pthread_mutex_unlock(&(file->mutex));

        // Traitement de la requête
        if (strstr(requete.message, "MESSAGE") == requete.message) {
            printf("\n\nRequête contient MESSAGE : %s\n", requete.message);
            // Si la requête commence par "MESSAGE", envoyer le message à tous les clients connectés
            pthread_mutex_lock(&mutex_clients);
            // Envoi de la réponse au client
            ssize_t nb_octets_envoyes = write(requete.client_socket, "0", strlen("0") + 1);
            if (nb_octets_envoyes == -1) {
                perror("Erreur lors de l'envoi de la réponse au client");
                pthread_mutex_unlock(&mutex_clients);
                continue; // Passer à la prochaine itération de la boucle
            }
            sleep(2);
            printf("Envoi du message à tous les clients (%d):\n", nombre_clients);
            for (int i = 0; i < nombre_clients; i++) {
                printf(" %d/%d : Envoi à %s\n", i, nombre_clients, inet_ntoa(clients[i].adresse.sin_addr)); // Afficher l'adresse IP du client auquel le message est envoyé
                ssize_t nb_octets_envoyes = write(clients[i].socket, requete.message, strlen(requete.message));
                if (nb_octets_envoyes == -1) {
                    perror("  Erreur lors de l'envoi du message au client");
                    continue;
                    // Il est préférable de continuer l'envoi aux autres clients même si une erreur se produit
                }
            }
            printf("Réponse envoyée à tous les utilisateurs.\n");
            
            pthread_mutex_unlock(&mutex_clients);
        } else {
            char command[TAILLEBUF * 2];
            snprintf(command, TAILLEBUF * 2, "./Gestion_requete %s %d", requete.message, port);

            int reponse_code = system(command); // Exécute le processus Gestion_requete.c

            char reponse[TAILLEBUF];

            if (WIFEXITED(reponse_code)) {
                int exit_status = WEXITSTATUS(reponse_code);
                if (exit_status == 3) {
                    printf("AFFICHER LISTE\n");
                    // La réponse est "3" pour retourner la liste à partir de la mémoire partagée
                    snprintf(reponse, TAILLEBUF, "%s", RetourListe(port));
                    strncpy(file->tab_message[file->debut].message, reponse, TAILLEBUF);
                } else {
                    // La réponse est retournée
                    snprintf(reponse, TAILLEBUF, "%d", exit_status);
                    strncpy(file->tab_message[file->fin].message, reponse, TAILLEBUF);
                }

                // Envoi de la réponse au client
                ssize_t nb_octets_envoyes = write(requete.client_socket, reponse, strlen(reponse) + 1);
                if (nb_octets_envoyes == -1) {
                    perror("erreur envoi réponse");
                }
            } else {
                // Erreur lors de l'exécution de Gestion_requete
                snprintf(reponse, TAILLEBUF, "Erreur lors de l'exécution de Gestion_requete");
                ssize_t nb_octets_envoyes = write(requete.client_socket, reponse, strlen(reponse) + 1);
                if (nb_octets_envoyes == -1) {
                    perror("erreur envoi réponse");
                }
            }

            // Affichage de la requête et de la réponse
            printf("Requête reçue du client %d : %s\n", requete.client_socket, requete.message);
            printf("Réponse envoyée au client %d : %s\n", requete.client_socket, reponse);
        }
        sleep(2);
    }
}

// Fonction exécutée par chaque thread client
void *ClientThread(void *arg) {
    ClientThreadArgs *args = (ClientThreadArgs *)arg;
    int client_socket = args->client_socket;
    FileMessage *file = args->file;
    free(arg);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr *)&client_addr, &client_addr_len);

    // Enregistrer le client
    AjouterClient(client_socket, client_addr);

    char message[TAILLEBUF];
    ssize_t nb_octets;

    // Lecture du message du client
    nb_octets = read(client_socket, message, TAILLEBUF);
    if (nb_octets == -1) {
        perror("erreur lecture message");
        close(client_socket);
        SupprimerClient(client_socket);
        return NULL;
    } else if (nb_octets == 0) {
        printf("Client %d déconnecté\n", client_socket);
        close(client_socket);
        SupprimerClient(client_socket);
        return NULL;
    }
    message[nb_octets] = '\0';

    // Ajouter la requête à la file de messages
    pthread_mutex_lock(&file->mutex);
    strncpy(file->tab_message[file->fin].message, message, TAILLEBUF);
    file->tab_message[file->fin].client_socket = client_socket;
    file->fin = (file->fin + 1) % MAX_CLIENTS;
    pthread_cond_signal(&file->cond);
    pthread_mutex_unlock(&file->mutex);

    return NULL;
}

// Fonction principale pour démarrer le serveur
void DemarrerServeur(int numero_port) {
    port = numero_port;

    // Créer un socket pour le serveur
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("erreur creation socket");
        exit(EXIT_FAILURE);
    }

    // Lier le socket à une adresse et un port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(numero_port);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("erreur liaison socket");
        exit(EXIT_FAILURE);
    }

    // Ecouter les connexions entrantes
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("erreur écoute");
        exit(EXIT_FAILURE);
    }

    // Initialisation de la file de message
    FileMessage file = {
        .tab_message = {0}, // Initialiser tous les éléments de la file de messages à 0
        .debut = 0,
        .fin = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER
    };

    // Créer un thread pour traiter les requêtes clients
    pthread_t tid;
    pthread_create(&tid, NULL, ProcessusRequetes, &file); // Passer l'adresse de la structure FileMessage comme argument

    printf("\nServeur démarré. \nEn attente de connexions sur le port %d...\n", numero_port);

    while (1) {
        // Accepter les connexions des clients
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("erreur acceptation connexion");
            continue;
        }

        // Créer un thread pour gérer la connexion client
        ClientThreadArgs *args = malloc(sizeof(ClientThreadArgs));
        args->client_socket = client_socket;
        args->file = &file; // Passer un pointeur vers la structure FileMessage
        pthread_t client_tid;
        pthread_create(&client_tid, NULL, ClientThread, args);
    }

    // Fermer le socket du serveur
    close(server_socket);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int numero_port = atoi(argv[1]);

    // Initialisation de la file de message
    FileMessage file = {
        //.tab_message = {0}, // Initialiser tous les éléments de la file de messages à 0
        .debut = 0,
        .fin = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER
    };

    // Ignorer le signal SIGPIPE pour éviter une terminaison inattendue lors de l'écriture sur un socket fermé
    signal(SIGPIPE, SIG_IGN);

    // Démarrer le serveur
    DemarrerServeur(numero_port);

    return 0;
}
