/*
 * serveur.c — Version 1 : serveur écho.
 *
 * Le serveur attend une connexion TCP, puis renvoie au client chaque
 * ligne reçue, à l'identique. Il traite UN SEUL client à la fois :
 * quand ce client part (/quit ou déconnexion), il en accepte un autre.
 *
 * Usage : ./serveur <port>
 */
#include "protocole.h"

#include <arpa/inet.h>    /* inet_ntop, htons, htonl */
#include <errno.h>
#include <netinet/in.h>   /* struct sockaddr_in */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>       /* close */

/* Nombre maximum de connexions en attente dans la file de listen(). */
#define FILE_ATTENTE 10

/* Message renvoyé au client quand il tape /quit (imposé par le sujet). */
#define MSG_AU_REVOIR "You will be terminated"

/*
 * Mis à 1 par le gestionnaire de SIGINT (Ctrl+C) pour demander l'arrêt.
 * "volatile sig_atomic_t" est le seul type qu'on a le droit de modifier
 * en toute sécurité depuis un gestionnaire de signal.
 */
static volatile sig_atomic_t arret_demande = 0;

/*
 * Socket du client en cours de traitement (-1 s'il n'y en a pas).
 * Le gestionnaire de SIGINT en a besoin pour débloquer la lecture.
 */
static volatile sig_atomic_t fd_client_courant = -1;

/*
 * gestion_sigint — gestionnaire du signal SIGINT (Ctrl+C).
 * Il lève le drapeau d'arrêt. Si un client est connecté, il "coupe"
 * aussi sa socket avec shutdown() : le recv() en cours retourne alors 0
 * et le serveur sort de sa boucle (sans cela, recevoir_tout() relancerait
 * recv() après l'interruption et attendrait le client indéfiniment).
 * shutdown() fait partie des fonctions autorisées dans un gestionnaire
 * de signal ("async-signal-safe"). La fermeture (close) reste faite dans main().
 * Paramètre : sig = numéro du signal (inutilisé).
 */
static void gestion_sigint(int sig)
{
    (void)sig;
    arret_demande = 1;
    if (fd_client_courant >= 0)
        shutdown(fd_client_courant, SHUT_RDWR);
}

/*
 * installer_signaux — ignore SIGPIPE et installe le gestionnaire de SIGINT.
 * On n'active PAS SA_RESTART : ainsi accept() et recv() sont interrompus
 * (errno = EINTR) quand on fait Ctrl+C, et la boucle peut s'arrêter.
 */
static void installer_signaux(void)
{
    struct sigaction sa;

    signal(SIGPIPE, SIG_IGN);   /* écrire sur une socket fermée ne tue plus le programme */

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestion_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

/*
 * creer_socket_ecoute — crée la socket du serveur et la met en écoute.
 * Paramètre : port = port TCP sur lequel écouter.
 * Retour : le descripteur de la socket d'écoute, ou -1 en cas d'erreur.
 */
static int creer_socket_ecoute(int port)
{
    int fd;
    int option = 1;
    struct sockaddr_in adresse;

    fd = socket(AF_INET, SOCK_STREAM, 0);          /* TCP sur IPv4 */
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    /* Permet de relancer le serveur tout de suite sur le même port. */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    memset(&adresse, 0, sizeof(adresse));
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = htonl(INADDR_ANY);   /* toutes les interfaces */
    adresse.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&adresse, sizeof(adresse)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    if (listen(fd, FILE_ATTENTE) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    return fd;
}

/*
 * gerer_client — dialogue avec un client jusqu'à son départ.
 * Chaque trame T_TEXTE reçue est renvoyée telle quelle dans une trame
 * T_SERVEUR. Sur "/quit", on envoie MSG_AU_REVOIR et on termine.
 * Paramètre : fd = socket connectée au client (fermée par l'appelant).
 */
static void gerer_client(int fd)
{
    /* +1 pour pouvoir ajouter un '\0' à la fin du texte reçu. */
    char donnees[MAX_PAYLOAD + 1];
    uint8_t type;
    uint32_t taille;
    int continuer = 1;

    while (continuer && !arret_demande) {
        int r = recevoir_trame(fd, &type, donnees, &taille);
        if (r <= 0) {
            /* 0 : le client a fermé ; -1 : erreur ou Ctrl+C sur le serveur. */
            if (arret_demande)
                printf("[serveur] Ctrl+C : fermeture de la connexion en cours.\n");
            else if (r == 0)
                printf("[serveur] Le client s'est déconnecté.\n");
            else if (!arret_demande)
                printf("[serveur] Erreur de réception, fin de la connexion.\n");
            continuer = 0;
        } else if (type != T_TEXTE) {
            printf("[serveur] Trame de type inattendu (%u) ignorée.\n", type);
        } else {
            donnees[taille] = '\0';
            if (strcmp(donnees, "/quit") == 0) {
                printf("[serveur] Le client a demandé /quit.\n");
                envoyer_trame(fd, T_SERVEUR, MSG_AU_REVOIR, strlen(MSG_AU_REVOIR));
                continuer = 0;
            } else {
                printf("[serveur] Reçu : \"%s\" -> renvoyé\n", donnees);
                /* Écho : on renvoie exactement les mêmes octets. */
                if (envoyer_trame(fd, T_SERVEUR, donnees, taille) < 0)
                    continuer = 0;
            }
        }
    }
}

/*
 * main — point d'entrée du serveur.
 * Vérifie les arguments, crée la socket d'écoute, puis accepte les
 * clients un par un jusqu'à Ctrl+C.
 */
int main(int argc, char *argv[])
{
    int port;
    int fd_ecoute;

    if (argc != 2) {
        fprintf(stderr, "Usage : %s <port>\n", argv[0]);
        fprintf(stderr, "Exemple : %s 8080\n", argv[0]);
        return EXIT_FAILURE;
    }
    port = analyser_port(argv[1]);
    if (port < 0) {
        fprintf(stderr, "Erreur : port invalide \"%s\" (attendu : 1024 à 65535).\n", argv[1]);
        return EXIT_FAILURE;
    }

    installer_signaux();

    fd_ecoute = creer_socket_ecoute(port);
    if (fd_ecoute < 0)
        return EXIT_FAILURE;

    printf("[serveur] En écoute sur le port %d (Ctrl+C pour arrêter).\n", port);
    fflush(stdout);

    while (!arret_demande) {
        struct sockaddr_in adr_client;
        socklen_t lg = sizeof(adr_client);
        char ip[INET_ADDRSTRLEN];
        int fd_client;

        fd_client = accept(fd_ecoute, (struct sockaddr *)&adr_client, &lg);
        if (fd_client < 0) {
            if (errno != EINTR)          /* EINTR = interrompu par Ctrl+C */
                perror("accept");
            continue;                    /* la condition de boucle décide */
        }

        inet_ntop(AF_INET, &adr_client.sin_addr, ip, sizeof(ip));
        printf("[serveur] Connexion de %s:%u\n", ip, ntohs(adr_client.sin_port));
        fflush(stdout);

        fd_client_courant = fd_client;   /* connu du gestionnaire de SIGINT */
        gerer_client(fd_client);
        fd_client_courant = -1;

        close(fd_client);
        printf("[serveur] Connexion avec %s:%u fermée. En attente d'un client...\n",
               ip, ntohs(adr_client.sin_port));
        fflush(stdout);
    }

    close(fd_ecoute);
    printf("\n[serveur] Arrêt propre du serveur.\n");
    return EXIT_SUCCESS;
}
