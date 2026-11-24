# Version 1 — Modèle client/serveur (écho)

## Ce que fait cette version
- Le **serveur** écoute sur un port TCP, accepte **un client à la fois** et lui renvoie chaque ligne reçue, à l'identique (écho).
- Le **client** se connecte (`port` puis `IP`), lit une ligne au clavier, l'envoie, puis affiche la réponse du serveur.
- `/quit` : le serveur répond `You will be terminated` et ferme la connexion. Le client affiche `Connection terminated`, ferme sa socket et se termine. Le serveur attend ensuite le client suivant.
- Le **protocole à trames** (`include/protocole.h`) est en place dès cette version : en-tête `type (1 o) + longueur (4 o, ordre réseau)`, puis les données.
- `Ctrl+C` sur le serveur l'arrête proprement, même si un client est connecté.

## Fichiers
| Fichier | Rôle |
|---|---|
| `include/protocole.h`, `src/protocole.c` | Format des trames, `envoyer_tout`, `recevoir_tout`, `envoyer_trame`, `recevoir_trame`, `analyser_port` |
| `src/serveur.c` | Socket d'écoute, boucle `accept`, écho, arrêt sur SIGINT |
| `src/client.c` | Connexion, saisie clavier, envoi et affichage de la réponse |

## Compiler et tester
```bash
make                      # produit ./serveur et ./client
./serveur 8080            # terminal 1   (ou : make run-serveur)
./client 8080 127.0.0.1   # terminal 2
```
Exemple de session côté client :
```
%> Hello World!
%> [Server] : Hello World!
%> /quit
%> [Server] : You will be terminated
%> Connection terminated
```
Tests automatiques (depuis la racine du projet) : `./tests/test_v1.sh`

## Choix et limites
- **Écho** : le serveur renvoie exactement les mêmes octets. C'est le **client** qui ajoute le préfixe d'affichage `[Server] : `.
- `Ctrl+D` côté client se comporte comme `/quit`. Les lignes vides ne sont pas envoyées.
- Le client est **séquentiel** (il envoie, puis attend la réponse). Si le serveur s'arrête, le client ne s'en aperçoit qu'à son prochain envoi. Les deux threads côté client arrivent en V2.
- Un seul client est servi à la fois : un 2ᵉ client qui se connecte attend dans la file de `listen()` que le 1er parte.
