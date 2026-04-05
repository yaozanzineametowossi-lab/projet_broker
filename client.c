/*
 * client.c - Client Broker Financier
 *
 * Concepts utilises : sockets TCP (AF_INET, SOCK_STREAM),
 * primitives : socket(), connect(), send(), recv(), close()
 *
 * Compilation : gcc client.c -o client
 * Execution   : ./client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP   "127.0.0.1"
#define PORT        8080
#define BUFFER_SIZE 1024

/*
 * Structure representant le portefeuille du client
 * (actions detenues et fonds disponibles)
 */
typedef struct {
    char nom[32];
    int  quantite;
} Ligne;

#define MAX_PORTEFEUILLE 20

Ligne portefeuille[MAX_PORTEFEUILLE];
int   nb_lignes  = 0;
float fonds      = 10000.0;  /* budget initial du client */

/* ------------------------------------------------------------------ */
/* Affiche le portefeuille actuel du client                            */
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

/*
 * Met a jour le portefeuille apres confirmation du serveur.
 * type_op : 'A' pour achat, 'V' pour vente
 */
void maj_portefeuille(const char *nom, int quantite, float prix_unitaire, char type_op) {
    int i;
    int trouve = 0;

    /* Chercher si le produit est deja en portefeuille */
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

    /* Nouveau produit lors d'un achat */
    if (!trouve && type_op == 'A' && nb_lignes < MAX_PORTEFEUILLE) {
        strncpy(portefeuille[nb_lignes].nom, nom, 31);
        portefeuille[nb_lignes].quantite = quantite;
        fonds -= quantite * prix_unitaire;
        nb_lignes++;
    }
}

/* ------------------------------------------------------------------ */
/* Analyse la reponse du serveur pour extraire prix et quantite        */
/* afin de mettre a jour le portefeuille local                         */
/* ------------------------------------------------------------------ */
void analyser_confirmation(const char *reponse, const char *nom, char type_op) {
    int   quantite     = 0;
    float prix_unitaire = 0.0;

    /* Chercher la ligne "Achat confirme" ou "Vente confirmee" */
    if (sscanf(reponse, "%*[^:]: %d x %*s a %f", &quantite, &prix_unitaire) == 2) {
        maj_portefeuille(nom, quantite, prix_unitaire, type_op);
    }
}

/* ------------------------------------------------------------------ */
/* Affiche le menu local du client                                     */
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

    /* 1. Creation de la socket TCP */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Erreur creation socket");
        exit(EXIT_FAILURE);
    }

    /* 2. Configuration de l'adresse du serveur */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Adresse IP invalide");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    /* 3. Connexion au serveur */
    if (connect(client_socket,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {
        perror("Erreur connexion au broker");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connecte au broker !\n\n");

    /* 4. Reception du message de bienvenue */
    memset(buffer_recep, 0, BUFFER_SIZE);
    octets_recus = recv(client_socket, buffer_recep, BUFFER_SIZE - 1, 0);
    if (octets_recus > 0) {
        printf("%s", buffer_recep);
    }

    afficher_menu_local();

    /* 5. Boucle principale : lecture saisie, envoi, reception reponse */
    while (1) {
        memset(buffer_envoi, 0, BUFFER_SIZE);

        /* Lire la commande saisie par l'utilisateur */
        if (fgets(buffer_envoi, BUFFER_SIZE, stdin) == NULL) {
            printf("\nFin de saisie (EOF). Deconnexion.\n");
            break;
        }

        /* Supprimer le retour a la ligne */
        buffer_envoi[strcspn(buffer_envoi, "\r\n")] = '\0';

        /* Ignorer les lignes vides */
        if (strlen(buffer_envoi) == 0) {
            printf("> ");
            fflush(stdout);
            continue;
        }

        /* Commande locale : affichage du portefeuille sans passer par le serveur */
        if (strcmp(buffer_envoi, "PORTEFEUILLE") == 0) {
            afficher_portefeuille();
            printf("> ");
            fflush(stdout);
            continue;
        }

       /* Verifier les fonds avant d'envoyer un ordre d'achat */
        if (strncmp(buffer_envoi, "ACHAT ", 6) == 0) {
            char nom[32];
            int  qte;
            if (sscanf(buffer_envoi + 6, "%31s %d", nom, &qte) == 2) {
                if (qte <= 0) {
                    printf("Quantite invalide.\n> ");
                    fflush(stdout);
                    continue;
                }
                /* Note : le prix sera verifie cote serveur.
                   On laisse le serveur valider le stock disponible.
                   La mise a jour des fonds locaux se fait apres confirmation. */
            }
        }

        /* Verifier le portefeuille avant d'envoyer un ordre de vente */
        if (strncmp(buffer_envoi, "VENTE ", 6) == 0) {
            char nom[32];
            int  qte;
            int  i;
            if (sscanf(buffer_envoi + 6, "%31s %d", nom, &qte) == 2) {
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

        /* Envoi de la commande au serveur */
        if (send(client_socket, buffer_envoi, strlen(buffer_envoi), 0) == -1) {
            perror("Erreur envoi");
            break;
        }

        /* Cas QUITTER : on sort apres envoi */
        if (strcmp(buffer_envoi, "QUITTER") == 0) {
            /* Recevoir le "Au revoir" du serveur */
            memset(buffer_recep, 0, BUFFER_SIZE);
            recv(client_socket, buffer_recep, BUFFER_SIZE - 1, 0);
            printf("%s", buffer_recep);
            break;
        }

        /* Reception de la reponse du serveur */
        memset(buffer_recep, 0, BUFFER_SIZE);
        octets_recus = recv(client_socket, buffer_recep, BUFFER_SIZE - 1, 0);

        if (octets_recus <= 0) {
            printf("Le serveur s'est deconnecte.\n");
            break;
        }

        printf("\n%s", buffer_recep);

        /* Mise a jour du portefeuille si achat ou vente confirmes */
        if (strncmp(buffer_envoi, "ACHAT ", 6) == 0 &&
            strstr(buffer_recep, "confirme") != NULL) {
            char nom[32];
            int  qte;
            float prix = 0.0;
            sscanf(buffer_envoi + 6, "%31s %d", nom, &qte);
            sscanf(buffer_recep, "%*[^:]: %d x %*s a %f", &qte, &prix);
            if (prix > 0) {
                maj_portefeuille(nom, qte, prix, 'A');
                printf("Portefeuille mis a jour. Fonds restants : %.2f USD\n", fonds);
            }
        } else if (strncmp(buffer_envoi, "VENTE ", 6) == 0 &&
                   strstr(buffer_recep, "confirmee") != NULL) {
            char nom[32];
            int  qte;
            float prix = 0.0;
            sscanf(buffer_envoi + 6, "%31s %d", nom, &qte);
            sscanf(buffer_recep, "%*[^:]: %d x %*s a %f", &qte, &prix);
            if (prix > 0) {
                maj_portefeuille(nom, qte, prix, 'V');
                printf("Portefeuille mis a jour. Fonds restants : %.2f USD\n", fonds);
            }
        }

        printf("> ");
        fflush(stdout);
    }

    /* 6. Fermeture de la socket */
    close(client_socket);
    printf("Deconnexion du broker. Au revoir !\n");
    return 0;
}
