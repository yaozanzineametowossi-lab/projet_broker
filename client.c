#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP   "127.0.0.1"
#define PORT        8080
#define BUFFER_SIZE 1024

typedef struct {
    char nom[32];
    int  quantite;
} Ligne;

#define MAX_PORTEFEUILLE 20

Ligne portefeuille[MAX_PORTEFEUILLE];
int   nb_lignes = 0;
float fonds     = 10000.0;  /* budget initial du client */