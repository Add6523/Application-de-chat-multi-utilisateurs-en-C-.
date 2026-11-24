# Chat client/serveur en C

> Application de chat multi-utilisateurs en C : sockets TCP, threads POSIX, salons et transfert de fichiers.

![Langage C](https://img.shields.io/badge/langage-C11-blue) ![Linux](https://img.shields.io/badge/OS-Linux-orange)

🚧 **Projet en cours de développement** : ce README sera complété à la fin du projet (démonstration, architecture, protocole, commandes).

## Évolution du projet
| Version | Contenu | Statut |
|---|---|---|
| [V1](v1/) | Client/serveur écho (1 client), protocole à trames | ✅ |
| V2 | Serveur multi-client (thread par client, 20 max) | ⏳ |
| V3 | Gestion des utilisateurs (`/nick`, `/who`, `/whois`) | ⏳ |
| V4 | Chat : broadcast, messages privés, salons | ⏳ |
| V5 | Transfert de fichiers relayé par le serveur | ⏳ |

## Compilation rapide
```bash
cd v1 && make
./serveur 8080              # dans un terminal
./client 8080 127.0.0.1     # dans un autre
```

## Auteurs
Sawadogo Adama et [binôme à compléter]. Projet de Programmation système, L3 Informatique, Université Thomas Sankara (UTS).
