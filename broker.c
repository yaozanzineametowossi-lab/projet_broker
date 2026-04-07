#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define PORT        8080
#define BUFFER_SIZE 1024
#define NB_PRODUITS 5
#define LOG_FILE    "broker.log"

/* ------------------------------------------------------------------ */
/* Structure representant un produit financier cote par le broker      */
/* ------------------------------------------------------------------ */
typedef struct {
    char   nom[32];
    float  prix;
    int    quantite_broker;  /* stock disponible chez le broker */
    float  fonds_broker;     /* liquidites du broker */
} Produit;

/* ------------------------------------------------------------------ */
/* YAO : stock de chaque produit mis a disposition pour achat/vente   */
/* ------------------------------------------------------------------ */
Produit catalogue[NB_PRODUITS] = {
    {"APPLE",    182.50, 100, 50000.0},
    {"GOOGLE",   140.30,  80, 50000.0},
    {"AMAZON",   178.90,  60, 50000.0},
    {"TESLA",    245.10,  50, 50000.0},
    {"MICROSOFT",415.60,  40, 50000.0}
};

/* ------------------------------------------------------------------ */
/* MICHEL : ecriture horodatee dans le fichier de log                  */
/*   - fflush() garantit l'ecriture immediate meme en cas de crash    */
/*   - printf() affiche aussi le log dans le terminal                 */
/* ------------------------------------------------------------------ */
void ecrire_log(const char *message) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        perror("Impossible d'ouvrir le fichier de log");
        return;
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char heure[20];
    strftime(heure, sizeof(heure), "%Y-%m-%d %H:%M:%S", tm_info);

    /* MICHEL : ecriture dans le fichier avec flush immediat */
    fprintf(f, "[%s] %s\n", heure, message);
    fflush(f);
    fclose(f);

    /* MICHEL : affichage dans le terminal */
    printf("[%s] %s\n", heure, message);
}

/* ------------------------------------------------------------------ */
/* Recherche d'un produit par son nom (insensible a la casse)          */
/* Retourne l'indice dans catalogue, ou -1 si non trouve               */
/* ------------------------------------------------------------------ */
int trouver_produit(const char *nom) {
    char nom_maj[32];
    int i, j;

    for (i = 0; nom[i] && i < 31; i++) {
        nom_maj[i] = (nom[i] >= 'a' && nom[i] <= 'z') ? nom[i] - 32 : nom[i];
    }
    nom_maj[i] = '\0';

    for (j = 0; j < NB_PRODUITS; j++) {
        if (strcmp(catalogue[j].nom, nom_maj) == 0) {
            return j;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* IZIDINE : envoie la liste de tous les produits disponibles au client*/
/* ------------------------------------------------------------------ */
void envoyer_catalogue(int client_socket) {
    char reponse[BUFFER_SIZE];
    char ligne[128];
    int i;

    strcpy(reponse, "--- Catalogue des produits ---\n");
    strcat(reponse, "NOM          PRIX      STOCK\n");
    strcat(reponse, "-----------------------------\n");

    for (i = 0; i < NB_PRODUITS; i++) {
        snprintf(ligne, sizeof(ligne), "%-12s %8.2f  %5d\n",
                 catalogue[i].nom,
                 catalogue[i].prix,
                 catalogue[i].quantite_broker);
        strcat(reponse, ligne);
    }

    /* IZIDINE : envoi des informations au client */
    send(client_socket, reponse, strlen(reponse), 0);
}

/* ------------------------------------------------------------------ */
/* IZIDINE : traite la demande d'info sur un produit et envoie         */
/*           les informations attendues au client                      */
/* ------------------------------------------------------------------ */
void traiter_info(int client_socket, const char *nom_produit) {
    char reponse[BUFFER_SIZE];
    int idx = trouver_produit(nom_produit);

    if (idx == -1) {
        snprintf(reponse, sizeof(reponse),
                 "Produit '%s' introuvable dans le catalogue.\n", nom_produit);
    } else {
        snprintf(reponse, sizeof(reponse),
                 "Produit  : %s\n"
                 "Prix     : %.2f USD\n"
                 "Stock    : %d actions\n",
                 catalogue[idx].nom,
                 catalogue[idx].prix,
                 catalogue[idx].quantite_broker);
    }

    /* IZIDINE : envoi de la reponse au client */
    send(client_socket, reponse, strlen(reponse), 0);
}

/* ------------------------------------------------------------------ */
/* Traite un ordre d'achat envoye par le client                        */
/* Syntaxe : ACHAT <NOM> <QUANTITE>                                    */
/*                                                                      */
/* IZIDINE : verifie que le stock du broker est suffisant              */
/* YAO     : met a jour le stock et les fonds apres validation         */
/* ------------------------------------------------------------------ */
void traiter_achat(int client_socket, const char *nom_produit, int quantite) {
    char reponse[BUFFER_SIZE];
    char log_msg[256];
    int idx = trouver_produit(nom_produit);

    if (idx == -1) {
        snprintf(reponse, sizeof(reponse),
                 "Produit '%s' introuvable.\n", nom_produit);
        send(client_socket, reponse, strlen(reponse), 0);
        return;
    }

    if (quantite <= 0) {
        strcpy(reponse, "Quantite invalide (doit etre > 0).\n");
        send(client_socket, reponse, strlen(reponse), 0);
        return;
    }

    /* IZIDINE : le broker ne peut pas vendre plus que son stock disponible */
    if (catalogue[idx].quantite_broker < quantite) {
        snprintf(reponse, sizeof(reponse),
                 "Stock insuffisant. Disponible : %d actions de %s.\n",
                 catalogue[idx].quantite_broker, catalogue[idx].nom);
        send(client_socket, reponse, strlen(reponse), 0);
        return;
    }

    /* YAO : validation de l'achat, mise a jour du stock et des fonds */
    float montant = quantite * catalogue[idx].prix;
    catalogue[idx].quantite_broker -= quantite;
    catalogue[idx].fonds_broker    += montant;

    snprintf(reponse, sizeof(reponse),
             "Achat confirme : %d x %s a %.2f USD/action\n"
             "Montant total  : %.2f USD\n"
             "Stock restant  : %d actions\n",
             quantite, catalogue[idx].nom, catalogue[idx].prix,
             montant, catalogue[idx].quantite_broker);

    send(client_socket, reponse, strlen(reponse), 0);

    snprintf(log_msg, sizeof(log_msg),
             "ACHAT : %d x %s (montant=%.2f)", quantite, catalogue[idx].nom, montant);
    ecrire_log(log_msg);
}

/* ------------------------------------------------------------------ */
/* Traite un ordre de vente envoye par le client                       */
/* Syntaxe : VENTE <NOM> <QUANTITE>                                    */
/*                                                                      */
/* YAO : verifie que le broker a les fonds pour racheter               */
/*       et met a jour le stock et les fonds apres validation          */
/* ------------------------------------------------------------------ */
void traiter_vente(int client_socket, const char *nom_produit, int quantite) {
    char reponse[BUFFER_SIZE];
    char log_msg[256];
    int idx = trouver_produit(nom_produit);

    if (idx == -1) {
        snprintf(reponse, sizeof(reponse),
                 "Produit '%s' introuvable.\n", nom_produit);
        send(client_socket, reponse, strlen(reponse), 0);
        return;
    }

    if (quantite <= 0) {
        strcpy(reponse, "Quantite invalide (doit etre > 0).\n");
        send(client_socket, reponse, strlen(reponse), 0);
        return;
    }

    float montant = quantite * catalogue[idx].prix;

    /* YAO : le broker ne peut pas racheter si ses fonds sont insuffisants */
    if (catalogue[idx].fonds_broker < montant) {
        snprintf(reponse, sizeof(reponse),
                 "Le broker n'a pas les fonds suffisants pour racheter %d x %s.\n",
                 quantite, catalogue[idx].nom);
        send(client_socket, reponse, strlen(reponse), 0);
        return;
    }

    /* YAO : validation de la vente, mise a jour du stock et des fonds */
    catalogue[idx].quantite_broker += quantite;
    catalogue[idx].fonds_broker    -= montant;

    snprintf(reponse, sizeof(reponse),
             "Vente confirmee : %d x %s a %.2f USD/action\n"
             "Montant recu   : %.2f USD\n"
             "Stock broker   : %d actions\n",
             quantite, catalogue[idx].nom, catalogue[idx].prix,
             montant, catalogue[idx].quantite_broker);

    send(client_socket, reponse, strlen(reponse), 0);

    snprintf(log_msg, sizeof(log_msg),
             "VENTE : %d x %s (montant=%.2f)", quantite, catalogue[idx].nom, montant);
    ecrire_log(log_msg);
}

/* ------------------------------------------------------------------ */
/* Envoie l'aide (liste des commandes disponibles) au client           */
/* ------------------------------------------------------------------ */
void envoyer_aide(int client_socket) {
    char aide[] =
        "--- Commandes disponibles ---\n"
        "CATALOGUE              : liste tous les produits\n"
        "INFO <nom>             : informations sur un produit\n"
        "ACHAT <nom> <quantite> : acheter des actions\n"
        "VENTE <nom> <quantite> : vendre des actions\n"
        "AIDE                   : afficher ce menu\n"
        "QUITTER                : se deconnecter\n"
        "-----------------------------\n";

    send(client_socket, aide, strlen(aide), 0);
}

/* ------------------------------------------------------------------ */
/* Gestion complete d'un client (appelee dans le processus fils)       */
/*                                                                      */
/* YAO     : reception des commandes (recv + parsing + dispatch)       */
/* IZIDINE : deconnexion propre (QUITTER) et inattendue (recv <= 0)   */
/* ------------------------------------------------------------------ */
void gerer_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char log_msg[256];
    int  octets_recus;

    char ip_client[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_client, INET_ADDRSTRLEN);

    snprintf(log_msg, sizeof(log_msg),
             "Nouveau client connecte : %s:%d",
             ip_client, ntohs(client_addr.sin_port));
    ecrire_log(log_msg);

    char bienvenue[] =
        "Bienvenue sur le Broker Financier !\n"
        "Tapez AIDE pour voir les commandes disponibles.\n";
    send(client_socket, bienvenue, strlen(bienvenue), 0);

    /* YAO : boucle de reception et traitement des commandes */
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        /* YAO : reception de la commande du client */
        octets_recus = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (octets_recus <= 0) {
            /* IZIDINE : deconnexion inattendue (coupure reseau, crash client) */
            snprintf(log_msg, sizeof(log_msg),
                     "Client deconnecte de facon inattendue : %s:%d",
                     ip_client, ntohs(client_addr.sin_port));
            ecrire_log(log_msg);
            break;
        }

        buffer[strcspn(buffer, "\r\n")] = '\0';

        snprintf(log_msg, sizeof(log_msg),
                 "Commande recue de %s : '%.80s'", ip_client, buffer);
        ecrire_log(log_msg);

        /* YAO : dispatch vers la fonction correspondante */
        if (strcmp(buffer, "CATALOGUE") == 0) {
            envoyer_catalogue(client_socket);

        } else if (strncmp(buffer, "INFO ", 5) == 0) {
            /* YAO : reception d'une demande d'information sur un produit */
            traiter_info(client_socket, buffer + 5);

        } else if (strncmp(buffer, "ACHAT ", 6) == 0) {
            char nom[32];
            int  qte;
            if (sscanf(buffer + 6, "%31s %d", nom, &qte) == 2) {
                traiter_achat(client_socket, nom, qte);
            } else {
                char err[] = "Syntaxe : ACHAT <nom> <quantite>\n";
                send(client_socket, err, strlen(err), 0);
            }

        } else if (strncmp(buffer, "VENTE ", 6) == 0) {
            char nom[32];
            int  qte;
            if (sscanf(buffer + 6, "%31s %d", nom, &qte) == 2) {
                traiter_vente(client_socket, nom, qte);
            } else {
                char err[] = "Syntaxe : VENTE <nom> <quantite>\n";
                send(client_socket, err, strlen(err), 0);
            }

        } else if (strcmp(buffer, "AIDE") == 0) {
            envoyer_aide(client_socket);

        } else if (strcmp(buffer, "QUITTER") == 0) {
            /* IZIDINE : deconnexion propre demandee par le client */
            char msg[] = "Au revoir !\n";
            send(client_socket, msg, strlen(msg), 0);
            snprintf(log_msg, sizeof(log_msg),
                     "Client %s a envoye QUITTER.", ip_client);
            ecrire_log(log_msg);
            break;

        } else {
            char err[] = "Commande inconnue. Tapez AIDE.\n";
            send(client_socket, err, strlen(err), 0);
        }
    }

    /* IZIDINE : fermeture de la socket du client */
    close(client_socket);
}

/* ------------------------------------------------------------------ */
/* YAO : nettoyage des processus fils zombies                          */
/* ------------------------------------------------------------------ */
void nettoyer_fils(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* ------------------------------------------------------------------ */
/* Point d'entree principal                                            */
/*                                                                      */
/* YAO : creation socket, bind, listen, accept, fork, multi-clients   */
/* ------------------------------------------------------------------ */
int main(void) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pid_t pid;

    /* YAO : installation du handler pour eviter les zombies */
    signal(SIGCHLD, nettoyer_fils);

    ecrire_log("Demarrage du serveur Broker...");

    /* YAO : 1. creation de la socket TCP */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Erreur creation socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* YAO : 2. configuration de l'adresse du serveur */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    /* YAO : 3. liaison de la socket a l'adresse et au port */
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erreur bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    /* YAO : 4. mise en ecoute */
    if (listen(server_socket, 5) == -1) {
        perror("Erreur listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg),
             "Broker en attente de connexions sur le port %d...", PORT);
    ecrire_log(log_msg);

    /* YAO : 5. boucle d'acceptation des connexions entrantes */
    while (1) {
        /* YAO : accepter la connexion d'un nouveau client */
        client_socket = accept(server_socket,
                               (struct sockaddr *)&client_addr,
                               &client_addr_len);

        if (client_socket == -1) {
            perror("Erreur accept");
            continue;
        }

        /* YAO : fork pour gerer ce client sans bloquer le serveur */
        pid = fork();

        if (pid == -1) {
            perror("Erreur fork");
            close(client_socket);

        } else if (pid == 0) {
            /* Processus fils : gere le client */
            close(server_socket);
            gerer_client(client_socket, client_addr);
            exit(EXIT_SUCCESS);

        } else {
            /* Processus pere : continue d'accepter de nouveaux clients */
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}
