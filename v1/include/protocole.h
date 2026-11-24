/*
 * protocole.h — Protocole de communication entre le client et le serveur.
 *
 * TCP transporte un FLUX d'octets, pas des messages séparés : deux send()
 * peuvent arriver collés dans un seul recv(), et un message peut arriver
 * coupé en plusieurs morceaux. On découpe donc le flux en "trames" :
 *
 *   +------------+------------------------+----------------------------+
 *   | type (1 o) | longueur (4 o, réseau) | données (longueur octets)  |
 *   +------------+------------------------+----------------------------+
 *
 * Le récepteur lit d'abord les 5 octets d'en-tête, ce qui lui dit
 * exactement combien d'octets de données il doit lire ensuite.
 */
#ifndef PROTOCOLE_H
#define PROTOCOLE_H

#include <stdint.h>
#include <sys/types.h>

/* Taille maximale des données d'une seule trame (en octets). */
#define MAX_PAYLOAD 4096

/* Types de trames (la liste s'agrandira au fil des versions). */
enum type_trame {
    T_TEXTE   = 1,  /* client -> serveur : ligne tapée par l'utilisateur      */
    T_SERVEUR = 2   /* serveur -> client : message du serveur à afficher      */
};

/*
 * envoyer_tout — envoie exactement n octets sur la socket fd.
 * send() peut n'envoyer qu'une partie des données : on boucle jusqu'à
 * ce que tout soit parti. MSG_NOSIGNAL évite de recevoir SIGPIPE si
 * le correspondant a fermé la connexion.
 * Paramètres : fd = socket, buf = données, n = nombre d'octets.
 * Retour : n si tout a été envoyé, -1 en cas d'erreur.
 */
ssize_t envoyer_tout(int fd, const void *buf, size_t n);

/*
 * recevoir_tout — reçoit exactement n octets depuis la socket fd.
 * Paramètres : fd = socket, buf = zone de réception (au moins n octets),
 *              n = nombre d'octets attendus.
 * Retour : n si tout a été reçu, 0 si le correspondant a fermé la
 *          connexion avant la fin, -1 en cas d'erreur.
 */
ssize_t recevoir_tout(int fd, void *buf, size_t n);

/*
 * envoyer_trame — envoie une trame complète (en-tête + données).
 * Paramètres : fd = socket, type = type de trame, donnees = contenu
 *              (peut être NULL si taille == 0), taille = nombre d'octets
 *              de données (<= MAX_PAYLOAD).
 * Retour : 0 si OK, -1 en cas d'erreur.
 */
int envoyer_trame(int fd, uint8_t type, const void *donnees, uint32_t taille);

/*
 * recevoir_trame — reçoit une trame complète.
 * Paramètres : fd = socket, type = (sortie) type reçu,
 *              donnees = (sortie) zone d'AU MOINS MAX_PAYLOAD octets,
 *              taille = (sortie) nombre d'octets de données reçus.
 * Attention : les données ne sont PAS terminées par '\0' (elles peuvent
 * être binaires). Pour du texte, prévoir MAX_PAYLOAD + 1 octets et
 * ajouter soi-même le '\0'.
 * Retour : 1 si OK, 0 si la connexion a été fermée, -1 si erreur
 *          (dont : trame annonçant plus de MAX_PAYLOAD octets).
 */
int recevoir_trame(int fd, uint8_t *type, void *donnees, uint32_t *taille);

/*
 * analyser_port — convertit le texte d'un numéro de port en entier.
 * Paramètre : texte = chaîne tapée par l'utilisateur (ex. "8080").
 * Retour : le port (1024 à 65535) si valide, -1 sinon.
 */
int analyser_port(const char *texte);

#endif /* PROTOCOLE_H */
