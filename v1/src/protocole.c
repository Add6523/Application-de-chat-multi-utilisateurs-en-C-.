/*
 * protocole.c — Implémentation des fonctions d'envoi et de réception
 * de trames (voir protocole.h pour le format).
 */
#include "protocole.h"

#include <arpa/inet.h>   /* htonl, ntohl */
#include <errno.h>
#include <stdlib.h>      /* strtol */
#include <string.h>      /* memcpy */
#include <sys/socket.h>  /* send, recv */

/* Taille de l'en-tête : 1 octet de type + 4 octets de longueur. */
#define TAILLE_ENTETE 5

/* Voir protocole.h */
ssize_t envoyer_tout(int fd, const void *buf, size_t n)
{
    const char *p = buf;      /* position courante dans le tampon */
    size_t reste = n;         /* nombre d'octets restant à envoyer */

    while (reste > 0) {
        ssize_t envoye = send(fd, p, reste, MSG_NOSIGNAL);
        if (envoye < 0) {
            if (errno == EINTR)   /* interrompu par un signal : on réessaie */
                continue;
            return -1;
        }
        p += envoye;
        reste -= (size_t)envoye;
    }
    return (ssize_t)n;
}

/* Voir protocole.h */
ssize_t recevoir_tout(int fd, void *buf, size_t n)
{
    char *p = buf;
    size_t reste = n;

    while (reste > 0) {
        ssize_t recu = recv(fd, p, reste, 0);
        if (recu < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (recu == 0)            /* le correspondant a fermé la connexion */
            return 0;
        p += recu;
        reste -= (size_t)recu;
    }
    return (ssize_t)n;
}

/* Voir protocole.h */
int envoyer_trame(int fd, uint8_t type, const void *donnees, uint32_t taille)
{
    unsigned char entete[TAILLE_ENTETE];
    uint32_t taille_reseau;

    if (taille > MAX_PAYLOAD)
        return -1;

    /* Construction de l'en-tête : type puis longueur en ordre réseau. */
    entete[0] = type;
    taille_reseau = htonl(taille);
    memcpy(entete + 1, &taille_reseau, sizeof(taille_reseau));

    if (envoyer_tout(fd, entete, TAILLE_ENTETE) < 0)
        return -1;
    if (taille > 0 && envoyer_tout(fd, donnees, taille) < 0)
        return -1;
    return 0;
}

/* Voir protocole.h */
int recevoir_trame(int fd, uint8_t *type, void *donnees, uint32_t *taille)
{
    unsigned char entete[TAILLE_ENTETE];
    uint32_t taille_reseau;
    uint32_t longueur;
    ssize_t r;

    /* 1) Lecture de l'en-tête de taille fixe. */
    r = recevoir_tout(fd, entete, TAILLE_ENTETE);
    if (r <= 0)
        return (int)r;            /* 0 = connexion fermée, -1 = erreur */

    *type = entete[0];
    memcpy(&taille_reseau, entete + 1, sizeof(taille_reseau));
    longueur = ntohl(taille_reseau);

    /* 2) On refuse une trame trop grande (protège notre tampon). */
    if (longueur > MAX_PAYLOAD)
        return -1;

    /* 3) Lecture des données annoncées. */
    if (longueur > 0) {
        r = recevoir_tout(fd, donnees, longueur);
        if (r <= 0)
            return (int)r;
    }
    *taille = longueur;
    return 1;
}

/* Voir protocole.h */
int analyser_port(const char *texte)
{
    char *fin;
    long port;

    errno = 0;
    port = strtol(texte, &fin, 10);
    /* Refus si : pas de chiffre, caractères en trop, débordement, hors plage. */
    if (fin == texte || *fin != '\0' || errno != 0 || port < 1024 || port > 65535)
        return -1;
    return (int)port;
}
