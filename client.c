#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP   "127.0.0.1"
#define PORT        8080
#define BUFFER_SIZE 1024

/* ------------------------------------------------------------------ */
/* MICHEL : structure representant une ligne du portefeuille client    */
/* ------------------------------------------------------------------ */
typedef struct {
    char nom[32];
    int  quantite;
} Ligne;

#define MAX_PORTEFEUILLE 20

/* MICHEL : portefeuille et fonds initiaux du client */
Ligne portefeuille[MAX_PORTEFEUILLE];
int   nb_lignes = 0;
float fonds     = 10000.0;  /* budget initial du client */

/* ------------------------------------------------------------------ */
/* MICHEL : affiche le portefeuille actuel du client dans le terminal  */
/* ------------------------------------------------------------------ */
void afficher_portefeuille(void) {
    int i;
    printf("\n--- Votre portefeuille ---\n");
    printf("Fonds disponibles : %.2f USD\n", fonds);
    if (nb_lignes == 0) {
        printf("Aucune action detenue.\n");
    } else {
        printf("%-12s  QUANTITE\n", "PRODUIT");
        printf("-------------------------\n");
        for (i = 0; i < nb_lignes; i++) {
            printf("%-12s  %d\n", portefeuille[i].nom, portefeuille[i].quantite);
        }
    }
    printf("--------------------------\n\n");
}

/* ------------------------------------------------------------------ */
/* MICHEL : met a jour le portefeuille apres confirmation du serveur   */
/*   type_op : 'A' pour achat, 'V' pour vente                         */
/* ------------------------------------------------------------------ */
void maj_portefeuille(const char *nom, int quantite, float prix_unitaire, char type_op) {
    int i;
    int trouve = 0;

    for (i = 0; i < nb_lignes; i++) {
        if (strcmp(portefeuille[i].nom, nom) == 0) {
            trouve = 1;
            if (type_op == 'A') {
                portefeuille[i].quantite += quantite;
                fonds -= quantite * prix_unitaire;
            } else {
                portefeuille[i].quantite -= quantite;
                fonds += quantite * prix_unitaire;
                if (portefeuille[i].quantite < 0) {
                    portefeuille[i].quantite = 0;
                }
            }
            break;
        }
    }

    /* Nouveau produit lors d'un premier achat */
    if (!trouve && type_op == 'A' && nb_lignes < MAX_PORTEFEUILLE) {
        strncpy(portefeuille[nb_lignes].nom, nom, 31);
        portefeuille[nb_lignes].quantite = quantite;
        fonds -= quantite * prix_unitaire;
        nb_lignes++;
    }
}

/* ------------------------------------------------------------------ */
/* MICHEL : affiche le menu des commandes disponibles dans le terminal */
/* ------------------------------------------------------------------ */
void afficher_menu_local(void) {
    printf("\n==============================\n");
    printf("  BROKER FINANCIER - CLIENT  \n");
    printf("==============================\n");
    printf("Commandes serveur :\n");
    printf("  CATALOGUE              - voir tous les produits\n");
    printf("  INFO <nom>             - details d'un produit\n");
    printf("  ACHAT <nom> <quantite> - acheter des actions\n");
    printf("  VENTE <nom> <quantite> - vendre des actions\n");
    printf("  AIDE                   - aide du serveur\n");
    printf("  QUITTER                - se deconnecter\n");
    printf("Commandes locales :\n");
    printf("  PORTEFEUILLE           - voir votre portefeuille\n");
    printf("==============================\n");
    printf("> ");
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* Point d'entree principal                                            */
/* ------------------------------------------------------------------ */
int main(void) {
    int    client_socket;
    struct sockaddr_in server_addr;
    char   buffer_envoi[BUFFER_SIZE];
    char   buffer_recep[BUFFER_SIZE];
    int    octets_recus;

    printf("Connexion au broker %s:%d...\n", SERVER_IP, PORT);

    /* MICHEL : 1. creation de la socket TCP */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Erreur creation socket");
        exit(EXIT_FAILURE);
    }

    /* MICHEL : 2. configuration de l'adresse du serveur */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Adresse IP invalide");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    /* MICHEL : 3. connexion au serveur broker */
    if (connect(client_socket,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {
        perror("Erreur connexion au broker");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connecte au broker !\n\n");

    /* MICHEL : reception et affichage du message de bienvenue */
    memset(buffer_recep, 0, BUFFER_SIZE);
    octets_recus = recv(client_socket, buffer_recep, BUFFER_SIZE - 1, 0);
    if (octets_recus > 0) {
        printf("%s", buffer_recep);
    }

    /* MICHEL : affichage de l'interface textuelle */
    afficher_menu_local();

    /* MICHEL : 4. boucle principale : saisie -> envoi -> reception -> affichage */
    while (1) {
        memset(buffer_envoi, 0, BUFFER_SIZE);

        /* MICHEL : lecture de la commande saisie par l'utilisateur */
        if (fgets(buffer_envoi, BUFFER_SIZE, stdin) == NULL) {
            printf("\nFin de saisie (EOF). Deconnexion.\n");
            break;
        }

        buffer_envoi[strcspn(buffer_envoi, "\r\n")] = '\0';

        if (strlen(buffer_envoi) == 0) {
            printf("> ");
            fflush(stdout);
            continue;
        }

        /* MICHEL : commande locale, pas transmise au serveur */
        if (strcmp(buffer_envoi, "PORTEFEUILLE") == 0) {
            afficher_portefeuille();
            printf("> ");
            fflush(stdout);
            continue;
        }

        /* IZIDINE : traitement d'un ordre d'achat */
        if (strncmp(buffer_envoi, "ACHAT ", 6) == 0) {
            char  nom[32];
            int   qte;
            if (sscanf(buffer_envoi + 6, "%31s %d", nom, &qte) == 2) {
                if (qte <= 0) {
                    printf("Quantite invalide.\n> ");
                    fflush(stdout);
                    continue;
                }
                /* MICHEL : verification des fonds disponibles avant envoi */
                /* (le prix exact sera confirme par le serveur)             */
            }
        }

        /* YAO : traitement d'un ordre de vente */
        if (strncmp(buffer_envoi, "VENTE ", 6) == 0) {
            char nom[32];
            int  qte;
            int  i;
            if (sscanf(buffer_envoi + 6, "%31s %d", nom, &qte) == 2) {
                /* MICHEL : verification que le client possede assez d'actions */
                int stock_client = 0;
                for (i = 0; i < nb_lignes; i++) {
                    if (strcmp(portefeuille[i].nom, nom) == 0) {
                        stock_client = portefeuille[i].quantite;
                        break;
                    }
                }
                if (qte > stock_client) {
                    printf("Vous ne possedez que %d actions de %s.\n", stock_client, nom);
                    printf("> ");
                    fflush(stdout);
                    continue;
                }
            }
        }

        /* MICHEL : envoi de la commande au serveur */
        if (send(client_socket, buffer_envoi, strlen(buffer_envoi), 0) == -1) {
            perror("Erreur envoi");
            break;
        }

        if (strcmp(buffer_envoi, "QUITTER") == 0) {
            memset(buffer_recep, 0, BUFFER_SIZE);
            recv(client_socket, buffer_recep, BUFFER_SIZE - 1, 0);
            printf("%s", buffer_recep);
            break;
        }

        /* MICHEL : reception et affichage de la reponse du serveur */
        memset(buffer_recep, 0, BUFFER_SIZE);
        octets_recus = recv(client_socket, buffer_recep, BUFFER_SIZE - 1, 0);

        if (octets_recus <= 0) {
            printf("Le serveur s'est deconnecte.\n");
            break;
        }

        /* MICHEL : affichage de la reponse dans l'interface textuelle */
        printf("\n%s", buffer_recep);

        /* IZIDINE : mise a jour du portefeuille apres achat confirme */
        if (strncmp(buffer_envoi, "ACHAT ", 6) == 0 &&
            strstr(buffer_recep, "confirme") != NULL) {
            char  nom[32];
            int   qte;
            float prix = 0.0;
            sscanf(buffer_envoi + 6, "%31s %d", nom, &qte);
            sscanf(buffer_recep, "%*[^:]: %d x %*s a %f", &qte, &prix);
            if (prix > 0) {
                maj_portefeuille(nom, qte, prix, 'A');
                /* MICHEL : affichage du solde restant */
                printf("Portefeuille mis a jour. Fonds restants : %.2f USD\n", fonds);
            }
        }

        /* YAO : mise a jour du portefeuille apres vente confirmee */
        else if (strncmp(buffer_envoi, "VENTE ", 6) == 0 &&
                 strstr(buffer_recep, "confirmee") != NULL) {
            char  nom[32];
            int   qte;
            float prix = 0.0;
            sscanf(buffer_envoi + 6, "%31s %d", nom, &qte);
            sscanf(buffer_recep, "%*[^:]: %d x %*s a %f", &qte, &prix);
            if (prix > 0) {
                maj_portefeuille(nom, qte, prix, 'V');
                /* MICHEL : affichage du solde restant */
                printf("Portefeuille mis a jour. Fonds restants : %.2f USD\n", fonds);
            }
        }

        printf("> ");
        fflush(stdout);
    }

    /* MICHEL : 5. fermeture de la socket */
    close(client_socket);
    printf("Deconnexion du broker. Au revoir !\n");
    return 0;
}
