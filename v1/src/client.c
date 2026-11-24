/*
 * client.c — Version 1 : client écho.
 *
 * Le client se connecte au serveur, lit une ligne au clavier, l'envoie,
 * puis attend la réponse du serveur (la même ligne) et l'affiche.
 * "/quit" termine la connexion proprement.
 *
 * Usage : ./client <port> <adresse_IP>   (ordre imposé par le sujet)
 */
#include "protocole.h"

#include <arpa/inet.h>    /* inet_pton, htons */
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>       /* close */

/* Invite affichée avant chaque saisie (comme dans le sujet). */
#define INVITE "%%> "   /* "%%" affiche un seul "%" avec printf */

/*
 * se_connecter — crée une socket TCP et la connecte au serveur.
 * Paramètres : port = port du serveur, ip = adresse IPv4 en texte.
 * Retour : la socket connectée, ou -1 en cas d'erreur.
 */
static int se_connecter(int port, const char *ip)
{
    int fd;
    struct sockaddr_in serveur;

    memset(&serveur, 0, sizeof(serveur));
    serveur.sin_family = AF_INET;
    serveur.sin_port = htons((uint16_t)port);
    /* inet_pton convertit "127.0.0.1" en adresse binaire ; retourne 1 si OK. */
    if (inet_pton(AF_INET, ip, &serveur.sin_addr) != 1) {
        fprintf(stderr, "Erreur : adresse IP invalide \"%s\".\n", ip);
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&serveur, sizeof(serveur)) < 0) {
        perror("connect (le serveur est-il démarré ?)");
        close(fd);
        return -1;
    }
    return fd;
}

/*
 * lire_ligne — affiche l'invite et lit une ligne au clavier.
 * Le '\n' final est retiré.
 * Paramètres : ligne = tampon de sortie, taille = sa taille.
 * Retour : 1 si une ligne a été lue, 0 en fin d'entrée (Ctrl+D).
 */
static int lire_ligne(char *ligne, size_t taille)
{
    printf(INVITE);         /* INVITE est un format printf, d'où le "%%" */
    fflush(stdout);
    if (fgets(ligne, (int)taille, stdin) == NULL)
        return 0;
    ligne[strcspn(ligne, "\n")] = '\0';   /* retire le '\n' s'il est présent */
    return 1;
}

/*
 * dialoguer — boucle principale : saisie, envoi, attente de la réponse.
 * Paramètre : fd = socket connectée au serveur.
 */
static void dialoguer(int fd)
{
    char ligne[MAX_PAYLOAD + 1];      /* ce que l'utilisateur tape    */
    char reponse[MAX_PAYLOAD + 1];    /* ce que le serveur renvoie    */
    uint8_t type;
    uint32_t taille;
    int continuer = 1;

    while (continuer) {
        int quitter;

        if (!lire_ligne(ligne, sizeof(ligne))) {
            /* Ctrl+D : on se comporte comme si l'utilisateur avait tapé /quit. */
            strcpy(ligne, "/quit");
            printf("%s\n", ligne);
        }
        if (ligne[0] == '\0')             /* ligne vide : on ne l'envoie pas */
            continue;

        quitter = (strcmp(ligne, "/quit") == 0);

        if (envoyer_trame(fd, T_TEXTE, ligne, strlen(ligne)) < 0) {
            continuer = 0;                /* serveur parti */
        } else if (recevoir_trame(fd, &type, reponse, &taille) <= 0) {
            continuer = 0;                /* connexion fermée ou erreur */
        } else {
            reponse[taille] = '\0';
            if (type == T_SERVEUR)
                printf(INVITE "[Server] : %s\n", reponse);
            if (quitter)
                continuer = 0;
        }
    }
    printf(INVITE "Connection terminated\n");
}

/*
 * main — vérifie les arguments, se connecte puis lance le dialogue.
 */
int main(int argc, char *argv[])
{
    int port;
    int fd;

    if (argc != 3) {
        fprintf(stderr, "Usage : %s <port> <adresse_IP>\n", argv[0]);
        fprintf(stderr, "Exemple : %s 8080 127.0.0.1\n", argv[0]);
        return EXIT_FAILURE;
    }
    port = analyser_port(argv[1]);
    if (port < 0) {
        fprintf(stderr, "Erreur : port invalide \"%s\" (attendu : 1024 à 65535).\n", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    fd = se_connecter(port, argv[2]);
    if (fd < 0)
        return EXIT_FAILURE;

    dialoguer(fd);

    close(fd);     /* fermeture de la socket ; aucune mémoire dynamique en V1 */
    return EXIT_SUCCESS;
}
