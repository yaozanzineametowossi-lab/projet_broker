#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9090
#define MAX_CLIENTS 10

int main(void){
    // Création du socket
    int serveur_socket =  socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM pour une connexion TCP fiable et ordonnée
    if (serveur_socket == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse d'écoute
    struct sockaddr_in adresse;
    adresse.in_family =  AF_INE; // AF_INET pour IPv4
    adresse.sin_addr.s_addr = INADDR_ANY; // ACcepter les connexions d'adresses différentes
    adresse.sin_port = htons(9090); // Conversion du port en format réseau

}