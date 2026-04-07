# Projet Broker Financier

Licence MIASHS - Programmation systèmes et réseaux L3 - 2025/2026

## Description

Simulation d'un broker financier en architecture client-serveur.  
Le serveur gère plusieurs clients simultanément via `fork()`.  
La communication repose sur des **sockets TCP** en langage C.

## Concepts utilisés (cours Chapitre 2 - Les Sockets)

- Sockets de domaine Internet `AF_INET` en mode connecté `SOCK_STREAM` (TCP)
- Primitives : `socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `close()`
- Conversion d'adresses : `htons()`, `inet_pton()`, `inet_ntop()`, `ntohs()`
- `fork()` pour gérer plusieurs clients simultanément
- Gestion des processus fils avec `SIGCHLD` / `waitpid()`

## Structure du projet

```
broker.c   - Code source du serveur broker
client.c   - Code source du client
broker.log - Fichier de log généré automatiquement au démarrage
README.md  - Ce fichier
```

## Compilation

```bash
gcc broker.c -o broker
gcc client.c -o client
```

## Exécution

### Lancer le serveur (dans un terminal)

```bash
./broker
```

Le serveur écoute sur le port **8080** et écrit les logs dans `broker.log`.

### Lancer un ou plusieurs clients (dans d'autres terminaux)

```bash
./client
```

## Commandes disponibles (côté client)

| Commande               | Description                          |
|------------------------|--------------------------------------|
| `CATALOGUE`            | Affiche tous les produits disponibles|
| `INFO <nom>`           | Détails d'un produit                 |
| `ACHAT <nom> <qte>`    | Acheter des actions au broker        |
| `VENTE <nom> <qte>`    | Vendre des actions au broker         |
| `AIDE`                 | Affiche l'aide du serveur            |
| `PORTEFEUILLE`         | Affiche le portefeuille local        |
| `QUITTER`              | Se déconnecter du broker             |

## Produits disponibles

| Produit   | Prix initial |
|-----------|-------------|
| APPLE     | 182.50 USD  |
| GOOGLE    | 140.30 USD  |
| AMAZON    | 178.90 USD  |
| TESLA     | 245.10 USD  |
| MICROSOFT | 415.60 USD  |

## Fonctionnalités implémentées

### Fonctionnalités simples
- Serveur multi-clients via `fork()`
- Réception et traitement des demandes d'information
- Envoi du catalogue et des détails d'un produit
- Gestion des déconnexions inattendues
- Logs horodatés dans un fichier `broker.log` (flush immédiat, survivant aux crashs)

### Fonctionnalités avancées
- Ordre d'achat : le client achète des actions au broker
- Ordre de vente : le client vend ses actions au broker
- Gestion du portefeuille côté client (actions détenues + fonds)

### Fonctionnalités avancées robustes
- Le broker ne peut pas vendre plus que son stock disponible
- Le broker ne peut pas racheter si ses fonds sont insuffisants
- Le client ne peut pas vendre plus que ce qu'il possède

## Exemple de session

```
> CATALOGUE
--- Catalogue des produits ---
NOM          PRIX      STOCK
-----------------------------
APPLE        182.50    100
...

> ACHAT APPLE 5
Achat confirme : 5 x APPLE a 182.50 USD/action
Montant total  : 912.50 USD
Stock restant  : 95 actions

> PORTEFEUILLE
--- Votre portefeuille ---
Fonds disponibles : 9087.50 USD
PRODUIT       QUANTITE
-------------------------
APPLE         5

> VENTE APPLE 2
Vente confirmee : 2 x APPLE a 182.50 USD/action
Montant recu   : 365.00 USD
```
