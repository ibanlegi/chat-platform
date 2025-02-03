#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>


#define TAILLEBUF 1000
#define PIPE_BUF_SIZE 100

typedef struct {
    int port;
    char *nom_serveur;
    char pseudo[TAILLEBUF];
    char password[TAILLEBUF];
    bool etat_co;
    int sockEcoute;
    int sockEnvoi;

    bool openChat;
    int pipe_fd;
    char *nom_tube;
    char *menu;
    pthread_t ThreadEcoute; // Thread pour la réception des messages
    pthread_t ThreadEnvoi;
    pthread_mutex_t mutex;   // Mutex pour la synchronisation
} Client;


// Déclarations des fonctions
void Gestionnaire_sigint(int signal);
int CreerSocket(Client *client);
void EnvoyerRequete(Client *client, int sock, char *requete);
void RecevoirReponse(Client *client, int sock, char *reponse);
void TraiterReponse(Client *client, char *requete, char *reponse);
char *MenuPremiereConnexion(Client *client);
char *MenuPrincipal(Client *client);
char *SaisirID(Client *client, char type);
char *EntreeValide(char *text_print);
void AfficherListeNumerotee(char *liste);
void *ThreadPrincipal(void *arg);
void *ThreadEcoute(void *arg);
void LancerAfficheur(Client *client);
void FermerAfficheur(Client *client);
void CreerTube(Client *client);
void EcrireTube(Client *client, const char *message);
void FermerTube(Client *client);

void Gestionnaire_sigint(int signal) {
    printf("\nCtrl+C détecté. Fermeture du programme.\n");
    exit(EXIT_SUCCESS);
}

int CreerSocket(Client *client) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Erreur de création de la socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in addr_serveur;
    struct hostent *host_serveur = gethostbyname(client->nom_serveur);
    if (host_serveur == NULL) {
        perror("Erreur de récupération de l'adresse serveur");
        exit(EXIT_FAILURE);
    }
    bzero((char *)&addr_serveur, sizeof(addr_serveur));
    addr_serveur.sin_family = AF_INET;
    addr_serveur.sin_port = htons(client->port);
    memcpy(&addr_serveur.sin_addr.s_addr, host_serveur->h_addr_list[0], host_serveur->h_length);
    if (connect(sock, (struct sockaddr *)&addr_serveur, sizeof(struct sockaddr_in)) == -1) {
        perror("Erreur de connexion au serveur");
        exit(EXIT_FAILURE);
    }
    return sock;
}

void EnvoyerRequete(Client *client, int sock, char *requete) {
    ssize_t nb_octets = write(sock, requete, strlen(requete) + 1);
    if (nb_octets == -1) {
        perror("Erreur d'envoi de la requête");
        exit(EXIT_FAILURE);
    }
    //Vérifier si la requête est QUITTER_pseudo
    if(strcmp(requete, "QUITTER_SORTIE") == 0){
        printf("Déconnexion du système.\n");
        exit(EXIT_SUCCESS);
    }
}

void RecevoirReponse(Client *client, int sock, char *reponse) {
    ssize_t nb_octets = read(sock, reponse, TAILLEBUF);
    if (nb_octets == -1) {
        perror("Erreur de lecture de la réponse");
        exit(EXIT_FAILURE);
    }
    reponse[nb_octets] = '\0';
}

void TraiterReponse(Client *client, char *requete, char *reponse) {
    char *token = strtok(requete, "_");
    if(strcmp(reponse, "2") == 0) {
        printf("\n> Echec : Le serveur a mis trop de temps pour répondre.\n");
    }else if(strcmp(reponse, "4") == 0){
        printf("\n> Echec : La fonctionnalité demandée n'a pas été implémentée.\n");
    }else if (!client->etat_co) {
        if (strcmp(token, "CONNEXION") == 0) {
            printf("\n> ");
            if (strcmp(reponse, "0") == 0) {
                printf("Connexion réussie!\n");
                client->etat_co = true;
                LancerAfficheur(client);
            } else {
                printf("La connexion a échoué.\n");
                strncpy(client->pseudo, "", TAILLEBUF);
                strncpy(client->password, "", TAILLEBUF);
            }
        } else if (strcmp(token, "CREERCOMPTE") == 0) {
            printf("\n> ");
            if (strcmp(reponse, "0") == 0) {
                printf("Compte créé avec succès! Veuillez vous connecter.\n");
                client->etat_co = false;
            } else {
                printf("La création de compte a échoué.\n");
            }
        }
    } else {
        if (strcmp(token, "DECONNEXION") == 0) {
            printf("\n> ");
            if (strcmp(reponse, "0") == 0) {
                printf("Déconnexion réussie!\n");
                client->etat_co = false;
                strncpy(client->pseudo, "", TAILLEBUF);
                strncpy(client->password, "", TAILLEBUF);
                FermerAfficheur(client);
            } else {
                printf("La déconnexion a échoué.\n");
            }
        } else if (strcmp(token, "SUPPRIMERCOMPTE") == 0) {
            printf("\n> ");
            if (strcmp(reponse, "0") == 0) {
                printf("Compte supprimé avec succès! Vous allez être déconnecté.\n");
                client->etat_co = false;
                strncpy(client->pseudo, "", TAILLEBUF);
                strncpy(client->password, "", TAILLEBUF);
                FermerAfficheur(client);
            } else {
                printf("La suppression de compte a échoué.\n");
            }
        } else if(strcmp(token, "LISTEUTILISATEUR") == 0) {
            AfficherListeNumerotee(reponse);
        } else if(strcmp(token, "MESSAGE") == 0) { //Etat de l'envoi du message
            printf("\n> ");
            if (strcmp(reponse, "0") == 0) {
                printf("Message envoyé avec succès!\n");
            } else {
                printf("Echec de l'envoi de message.\n");
                printf("Résultat = %s\n", reponse);
            }
        }
    }
    if (strcmp(token, "QUITTER") == 0) {
        printf("\n> ");
        if (strcmp(reponse, "0") == 0) {
            printf("Déconnexion du système.\n");
            client->etat_co = false;
            FermerAfficheur(client);
            //sleep(1);
            exit(EXIT_SUCCESS);
        } else {
            printf("La déconnexion n'a pas pu se faire.\n");
        }
    }
}

char *EntreeValide(char *text_print) {
    char *entree = (char *)malloc(TAILLEBUF * sizeof(char)); // Allouer de la mémoire
    if (entree == NULL) {
        fprintf(stderr, "Erreur d'allocation de mémoire.\n");
        exit(EXIT_FAILURE);
    }
    char *pos;
    do {
        printf("%s", text_print);
        fgets(entree, TAILLEBUF, stdin);
        if ((pos = strchr(entree, '_')) != NULL) {
            printf("> Le caractère '_' est interdit. Veuillez saisir à nouveau.\n");
        }
    } while (pos != NULL);
    return entree;
}

char *SaisirID(Client *client, char type) {
    char msg[TAILLEBUF];
    char entree[TAILLEBUF];
    switch (type) {
        case 'E': // Enregistrer ID
            strncpy(entree, EntreeValide("Nom d'utilisateur : "), TAILLEBUF);
            entree[strcspn(entree, "\n")] = '\0'; // Supprimer le '\n' de la fin
            strncpy(client->pseudo, entree, TAILLEBUF);
            strncpy(msg, entree, TAILLEBUF); // Réinitialise la requête
            strncat(msg, "_", TAILLEBUF - strlen(msg) - 1); // Ajouter un '_'
            strncpy(entree, EntreeValide("Mot de passe : "), TAILLEBUF);
            entree[strcspn(entree, "\n")] = '\0'; // Supprimer le '\n' de la fin
            strncpy(client->password, entree, TAILLEBUF);
            strncat(msg, entree, TAILLEBUF - strlen(msg) - 1);
            break;
        case 'R': // Retourner ID
            strncpy(entree, client->pseudo, TAILLEBUF);
            entree[strcspn(entree, "\n")] = '\0'; // Supprimer le '\n' de la fin
            strncpy(msg, entree, TAILLEBUF); // Réinitialise la requête
            strncat(msg, "_", TAILLEBUF - strlen(msg) - 1); // Ajouter un '_'
            strncpy(entree, client->password, TAILLEBUF);
            entree[strcspn(entree, "\n")] = '\0'; // Supprimer le '\n' de la fin
            strncat(msg, entree, TAILLEBUF - strlen(msg) - 1);
            break;
        case 'V': //Retourner avec vérification du password
            strncpy(entree, client->pseudo, TAILLEBUF);
            entree[strcspn(entree, "\n")] = '\0'; // Supprimer le '\n' de la fin
            strncpy(msg, entree, TAILLEBUF); // Réinitialise la reqûête
            strncat(msg, "_", TAILLEBUF - strlen(msg) - 1); // Ajouter un '_'
            strncpy(entree, EntreeValide("Mot de passe : "), TAILLEBUF);
            entree[strcspn(entree, "\n")] = '\0';
            while (strcmp(client->password, entree) != 0){
                printf("Mot de passe incorrect\n");
                strncpy(entree, EntreeValide("Mot de passe : "), TAILLEBUF);
                entree[strcspn(entree, "\n")] = '\0';
            }
            strncat(msg, entree, TAILLEBUF - strlen(msg) - 1);
            break;
        default:
            fprintf(stderr, "Type de saisie invalide.\n");
            exit(EXIT_FAILURE);
    }
    return strdup(msg);
}

char *MenuPremiereConnexion(Client *client) {
    static char requete[TAILLEBUF];
    char choix[TAILLEBUF];
    bool choixValide = false;
    while (!choixValide) {
        printf("\n==============================\n");
        printf("             Menu             \n");
        printf("==============================\n");
        printf("1 - Se connecter\n");
        printf("2 - Créer un compte\n");
        printf("3 - Quitter\n");
        printf("------------------------------\n");
        printf("Choix : ");
        fgets(choix, TAILLEBUF, stdin);
        switch (atoi(choix)) {
            case 1:
                snprintf(requete, TAILLEBUF, "CONNEXION_");
                printf("\n==============================\n");
                printf("           Connexion          \n");
                printf("==============================\n");
                choixValide = true;
                strncat(requete, SaisirID(client, 'E'), TAILLEBUF - strlen(requete) - 1);
                break;
            case 2:
                snprintf(requete, TAILLEBUF, "CREERCOMPTE_");
                printf("\n==============================\n");
                printf("        Créer un compte       \n");
                printf("==============================\n");
                choixValide = true;
                strncat(requete, SaisirID(client, 'E'), TAILLEBUF - strlen(requete) - 1);
                break;
            case 3:
                snprintf(requete, TAILLEBUF, "QUITTER_");
                choixValide = true;
                strncat(requete, "SORTIE", TAILLEBUF - strlen(requete) - 1);
                break;
            default:
                printf("> Choix invalide. Veuillez choisir une option valide.\n");
                break;
        }
    }
    return requete;
}

char *MenuPrincipal(Client *client) {
    static char requete[TAILLEBUF];
    char choix[TAILLEBUF];
    bool choixValide = false;
    while (!choixValide) {
        printf("\n==============================\n");
        printf("             Menu             \n");
        printf("==============================\n");
        printf("Connecté en tant que @%s\n\n", client->pseudo);
        printf("1 - Afficher la liste des utilisateurs présents\n");
        printf("2 - Envoyer un message\n");
        printf("3 - Se déconnecter\n");
        printf("4 - Supprimer le compte\n");
        printf("5 - Quitter\n");
        printf("------------------------------\n");
        printf("Choix : ");
        fgets(choix, TAILLEBUF, stdin);
        switch (atoi(choix)) {
            case 1:
                snprintf(requete, TAILLEBUF, "LISTEUTILISATEUR_");
                choixValide = true;
                break;
            case 2:
                snprintf(requete, TAILLEBUF*2, "MESSAGE_@%s > ", client->pseudo);
                printf("@%s > ", client->pseudo);
                char entree[TAILLEBUF];
                fgets(entree, TAILLEBUF, stdin);
                strncat(requete, entree, TAILLEBUF - strlen(requete) - 1);
                choixValide = true;                
                break;
            case 3:
                snprintf(requete, TAILLEBUF, "DECONNEXION_");
                strncat(requete, SaisirID(client, 'R'), TAILLEBUF - strlen(requete) - 1);
                choixValide = true;
                break;
            case 4:
                snprintf(requete, TAILLEBUF, "SUPPRIMERCOMPTE_");
                strncat(requete, SaisirID(client, 'V'), TAILLEBUF - strlen(requete) - 1);
                choixValide = true;
                break;
            case 5:
                snprintf(requete, TAILLEBUF, "QUITTER_");
                strncat(requete, SaisirID(client, 'R'), TAILLEBUF - strlen(requete) - 1);
                choixValide = true;
                break;
            default:
                printf("> Choix invalide. Veuillez choisir une option valide.\n");
                break;
        }
    }
    return requete;
}

void AfficherListeNumerotee(char *liste) {
    char *token;
    int numero = 1;
    // Utilisation de strtok pour séparer la liste en pseudonymes individuels
    token = strtok(liste, "_");
    while (token != NULL) {
        printf("%d - @%s\n", numero++, token);
        token = strtok(NULL, "_");
    }
}

// Thread principal pour la communication avec le serveur
void *ThreadPrincipal(void *arg) {
    Client *client = (Client *)arg;

    while(1){
        client->sockEnvoi = CreerSocket(client);
        // Menu en fonction de l'état de connexion du client
        if (!client->etat_co) {
            client->menu = MenuPremiereConnexion(client);
        } else {
            client->menu = MenuPrincipal(client);
        }
        // Envoi de la requête
        pthread_mutex_lock(&(client->mutex));
        EnvoyerRequete(client, client->sockEnvoi, client->menu);
        
        // Réception de la réponse
        char reponse[TAILLEBUF];
        RecevoirReponse(client, client->sockEnvoi, reponse);
        pthread_mutex_unlock(&(client->mutex));

        TraiterReponse(client, client->menu, reponse);

        close(client->sockEnvoi);
        
    }
    
    return NULL;
}



void *ThreadEcoute(void *arg) {
    Client *client = (Client *)arg;
    char reponse[TAILLEBUF];
    while (1) {
        RecevoirReponse(client, client->sockEcoute, reponse);
        char *message = strtok(reponse, "_");
        if(client->openChat){
            if(strstr(message, "MESSAGE") != NULL){
            message = strtok(NULL, "_");
            char *msg_envoi;
            EcrireTube(client, message);
            }
        }
        
    }
    return NULL;
}

char *GenererNomTube(int pid) {
    char *nom_tube = (char *)malloc(20 * sizeof(char));
    if (nom_tube == NULL) {
        fprintf(stderr, "Erreur d'allocation de mémoire.\n");
        exit(EXIT_FAILURE);
    }
    snprintf(nom_tube, 20, "pipe_chat_%d", pid);
    return nom_tube;
}

// Fonction pour créer un tube de communication spécifique au client
void CreerTube(Client *client) {
    client->nom_tube = GenererNomTube(getpid());
    client->pipe_fd = open(client->nom_tube, O_WRONLY | O_CREAT, 0644); // Ouvrir le tube en écriture
    if (client->pipe_fd == -1) {
        perror("Erreur lors de l'ouverture du tube de communication");
        exit(EXIT_FAILURE);
    }
}

// Fonction pour fermer le tube de communication spécifique au client
void FermerTube(Client *client) {
    close(client->pipe_fd);
    if (remove(client->nom_tube) == 0) {
        //printf("Le fichier \"%s\" a été supprimé avec succès.\n", client->nom_tube);
    } else {
        perror("Erreur lors de la suppression du fichier");
    }
}

// Fonction pour écrire dans le tube de communication
void EcrireTube(Client *client, const char *message) {
    if (write(client->pipe_fd, message, strlen(message) + 1) == -1) {
        perror("Erreur lors de l'écriture dans le tube de communication");
        exit(EXIT_FAILURE);
    }
}

void LancerAfficheur(Client *client) {
    CreerTube(client);
    char commande[100];
    int ret = sprintf(commande, "gnome-terminal -- /bin/bash -c './Afficheur_message %s; exec /bin/bash'", client->nom_tube);
    if (ret < 0) {
        printf("Error in sprintf()\n");
        exit(1);
    }

    pid_t pid = fork(); // Création d'un nouveau processus
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) { // Code du fils
        execl("/usr/bin/gnome-terminal", "gnome-terminal", "--", "/bin/bash", "-c", commande, NULL);
        perror("execl");
        exit(1);
    }
    client->openChat = true;
}

void FermerAfficheur(Client *client){
    EcrireTube(client, "exit");
    sleep(1);
    if (ftruncate(client->pipe_fd, 0) == -1) {
        perror("Erreur lors de la troncature du fichier");
        exit(EXIT_FAILURE);
    }
    client->openChat = false;

    FermerTube(client);
}

// Fonction principale
int main(int argc, char *argv[]) {
    // Vérification du nombre d'arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <nom_serveur> <port_serveur>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Initialisation du client
    Client client;
    client.nom_serveur = argv[1];
    client.port = atoi(argv[2]);
    client.etat_co = false;
    client.openChat = false;

    // Initialisation du mutex pour la synchronisation
    if (pthread_mutex_init(&(client.mutex), NULL) != 0) {
        fprintf(stderr, "Erreur lors de l'initialisation du mutex.\n");
        exit(EXIT_FAILURE);
    }

    // Événements de fermeture imprévue (Ctrl+C)
    if (signal(SIGINT, Gestionnaire_sigint) == SIG_ERR) {
        fprintf(stderr, "Erreur lors de l'enregistrement du gestionnaire de signal.\n");
        exit(EXIT_FAILURE);
    }

    //client.sockEnvoi = CreerSocket(&client);
    // Création du thread pour envoyer les requêtes
    if (pthread_create(&(client.ThreadEnvoi), NULL, ThreadPrincipal, (void *)&client) != 0) {
        fprintf(stderr, "Erreur lors de la création du thread pour envoyer les requêtes.\n");
        exit(EXIT_FAILURE);
    }

    client.sockEcoute = CreerSocket(&client);
    // Création du thread pour écouter les réponses du serveur
    if (pthread_create(&(client.ThreadEcoute), NULL, ThreadEcoute, (void *)&client) != 0) {
        fprintf(stderr, "Erreur lors de la création du thread pour écouter les réponses du serveur.\n");
        exit(EXIT_FAILURE);
    }


    // Attente de la fin du thread d'envoi
    pthread_join(client.ThreadEnvoi, NULL);
    pthread_join(client.ThreadEcoute, NULL);
    close(client.sockEcoute);

    FermerAfficheur(&client);

    return 0;
}
