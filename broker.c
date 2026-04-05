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

typedef struct {
    char   nom[32];
    float  prix;
    int    quantite_broker;  /* stock disponible chez le broker */
    float  fonds_broker;     /* liquidites du broker */
} Produit;

Produit catalogue[NB_PRODUITS] = {
    {"APPLE",    182.50, 100, 50000.0},
    {"GOOGLE",   140.30,  80, 50000.0},
    {"AMAZON",   178.90,  60, 50000.0},
    {"TESLA",    245.10,  50, 50000.0},
    {"MICROSOFT",415.60,  40, 50000.0}
};

void ecrire_log(){}

int trouver_produit(const char *nom){}

void envoyer_catalogue(int client_socket) {}

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

void nettoyer_fils(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

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