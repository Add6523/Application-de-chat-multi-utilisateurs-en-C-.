# CLAUDE.md — Projet Programmation Système : Chat client/serveur en C

> Ce fichier est le cahier des charges et le guide de travail pour Claude Code.
> Lis-le entièrement avant d'écrire la moindre ligne de code, et relis la section
> de la version en cours avant de commencer chaque version.

---

## 1. Contexte

- **Cours** : ECU Programmation système, Université Thomas Sankara (UTS), Licence 3 Informatique.
- **Enseignant** : Dr Moustapha BIKIENGA (bmoustaph@yahoo.fr).
- **Travail individuel** : Sawadogo Adama, seul (pas de binôme). GitHub : [Add6523](https://github.com/Add6523).
- **Objectif** : réaliser en **langage C**, sous **Linux (Ubuntu)**, une application de **chat client/serveur** en TCP permettant :
  - d'échanger des messages entre 2 utilisateurs (privé), entre plusieurs (salon) ou avec tous les connectés (broadcast) ;
  - de s'envoyer des fichiers.
- **Méthode imposée** : développement **incrémental** en 5 versions. **Chaque version doit être archivée** (un dossier par version + un tag Git).

### Le développeur (Adama) est étudiant : il doit comprendre le code
C'est un projet universitaire qu'Adama devra présenter et défendre. Donc :
- Le code doit être **simple, lisible et abondamment commenté en français** (chaque fonction a un commentaire d'en-tête : rôle, paramètres, valeur de retour).
- Pas d'astuces obscures, pas de bibliothèques externes : uniquement la libc POSIX et `pthread`.
- **À la fin de chaque version**, tu t'arrêtes et tu donnes à Adama :
  1. un résumé de ce qui a été fait ;
  2. une explication courte des notions clés de la version (ex. : `accept()`, `pthread_create`, mutex…) ;
  3. les commandes exactes pour compiler et tester lui-même.
- **Ne passe jamais à la version suivante sans qu'Adama l'ait validée.**

---

## 2. Choix techniques (déjà validés, ne pas changer)

| Sujet | Choix |
|---|---|
| Langage / compilateur | C (C11), `gcc -Wall -Wextra -pthread -g` |
| Système | Ubuntu Linux |
| Concurrence serveur | **Un thread par client** (`pthread_create` + `pthread_detach`) |
| Protection des données partagées | **Mutex** (`pthread_mutex_t`) — vu en cours |
| Concurrence client | **2 threads** : thread principal = lecture clavier + envoi ; thread secondaire = réception + affichage |
| `select()` / `poll()` | **Ne pas utiliser** (pas vu en cours) |
| Transport | TCP, IPv4 |
| Transfert de fichiers (V5) | **Relayé par le serveur** (pas de connexion directe client↔client) |
| Langue | Code (noms de variables) en français ou anglais, au choix mais cohérent ; **commentaires et messages en français**, sauf les messages imposés par le sujet (voir plus bas) qui restent en anglais |

### Lancement des programmes
```
./serveur <port>
./client <port> <adresse_IP>        # ordre imposé par le sujet : port PUIS IP
```
Exemple : `./serveur 8080` puis `./client 8080 127.0.0.1`.
Vérifier les arguments (nombre, port valide 1024–65535, IP valide via `inet_pton`) et afficher un message d'usage clair sinon.

---

## 3. Organisation du dépôt

Le dossier racine est celui qu'Adama a ouvert dans Claude Code. Tu crées cette arborescence :

```
<racine>/
├── CLAUDE.md                 ← ce fichier
├── README.md                 ← vitrine GitHub (voir §9)
├── .gitignore                ← exclut les exécutables et *.o, fichiers reçus, etc.
├── v1/                       ← Version 1 : client/serveur écho
│   ├── Makefile
│   ├── README.md             ← ce que fait cette version + comment tester
│   ├── include/              ← fichiers .h
│   └── src/                  ← fichiers .c (serveur.c, client.c, protocole.c…)
├── v2/                       ← Version 2 : multi-client (copie de v1 puis évolution)
├── v3/
├── v4/
├── v5/                       ← version finale
├── tests/                    ← scripts de test bash (un par version)
├── docs/
│   ├── rapport.md            ← rapport du projet (Adama le modifiera ensuite)
│   ├── manuel_utilisation.md ← manuel utilisateur
│   └── images/               ← captures / schémas éventuels
└── livrable/
    └── creer_archive.sh      ← construit l'archive ZIP à envoyer au prof
```

Règles :
- **Chaque version est autonome** : on peut faire `cd v3 && make` sans dépendre des autres dossiers. Pour commencer la version N, copie le dossier de la version N-1, puis fais-le évoluer. **Ne modifie plus jamais une version déjà validée** (sauf correction de bug demandée par Adama).
- Chaque `Makefile` fournit les cibles : `all` (produit `serveur` et `client` dans le dossier de la version), `clean`, `run-serveur` (port 8080 par défaut).
- Compilation **sans aucun warning** avec `-Wall -Wextra`.

---

## 4. Le protocole de communication (à mettre en place dès la V1)

### Pourquoi
TCP est un **flux d'octets**, pas une suite de messages : deux `send()` peuvent arriver collés en un seul `recv()`, ou un message peut arriver coupé en deux. Il faut donc **délimiter les messages**. On utilise un en-tête de taille fixe, ce qui permet aussi d'envoyer des données binaires (fichiers en V5).

### Format d'une trame
```
+------------+-----------------------+---------------------------+
| type (1 o) | longueur (4 o, réseau)| données (longueur octets) |
+------------+-----------------------+---------------------------+
```
- `type` : `uint8_t` (voir l'énumération ci-dessous).
- `longueur` : `uint32_t` en **ordre réseau** (`htonl` / `ntohl`).
- Taille maximale des données d'une trame : `MAX_PAYLOAD = 4096` octets. Rejeter toute trame annonçant plus.

### Fichiers communs (`include/protocole.h`, `src/protocole.c`)
- `ssize_t envoyer_tout(int fd, const void *buf, size_t n)` : boucle sur `send()` jusqu'à tout envoyer (gère les envois partiels et `EINTR`). Utiliser `MSG_NOSIGNAL` pour ne pas recevoir `SIGPIPE`.
- `ssize_t recevoir_tout(int fd, void *buf, size_t n)` : boucle sur `recv()` jusqu'à tout recevoir ; retourne 0 si le correspondant a fermé la connexion.
- `int envoyer_trame(int fd, uint8_t type, const void *donnees, uint32_t taille)`.
- `int recevoir_trame(int fd, uint8_t *type, void *donnees, uint32_t *taille)` → retourne 1 si OK, 0 si connexion fermée, -1 si erreur.
- Ignorer `SIGPIPE` aussi via `signal(SIGPIPE, SIG_IGN)` dans le serveur et le client.

### Types de trames (énumération à faire grandir au fil des versions)
| Type | Sens | Rôle | Version |
|---|---|---|---|
| `T_TEXTE` | client → serveur | ligne tapée par l'utilisateur (texte ou commande `/…`) | V1 |
| `T_SERVEUR` | serveur → client | message/réponse du serveur à afficher | V1 |
| `T_REFUS` | serveur → client | connexion refusée (serveur plein) | V2 |
| `T_BYE` | serveur → client | fin de connexion suite à `/quit` | V2 |
| `T_FICHIER_OFFRE` | les deux sens | proposition d'envoi (nom, taille, expéditeur/destinataire) | V5 |
| `T_FICHIER_ACCEPTE` / `T_FICHIER_REFUSE` | les deux sens | réponse du récepteur | V5 |
| `T_FICHIER_DONNEES` | les deux sens | morceau de fichier (≤ 4096 o) | V5 |
| `T_FICHIER_FIN` | les deux sens | fin du fichier | V5 |
| `T_FICHIER_CONFIRME` | les deux sens | confirmation de réception complète | V5 |

Principe simple : **le client envoie les lignes tapées telles quelles dans `T_TEXTE`, c'est le serveur qui analyse les commandes.** Seules les commandes qui touchent aux fichiers (`/sendfile`, `/accept`, `/refuse`) et `/quit` ont aussi un traitement côté client.

---

## 5. Architecture des threads et des mutex

### Serveur
```
main()
 ├─ socket(), setsockopt(SO_REUSEADDR), bind(), listen()
 └─ boucle infinie :
      fd = accept()
      lock(mutex_clients)
      si nb_clients >= MAX_CLIENTS (20) :
           unlock ; envoyer T_REFUS "Server cannot accept incoming connections anymore. Try again later." ; close(fd)
      sinon :
           allouer (malloc) une struct client, l'ajouter au tableau, nb_clients++
           unlock
           pthread_create(gerer_client, client) puis pthread_detach

gerer_client(client)
 └─ boucle : recevoir_trame() → traiter → répondre
      à la sortie (/quit, déconnexion brutale, erreur) :
           retirer le client de tous les salons (V4) puis du tableau (sous mutex)
           close(fd), free(client), fin du thread
```

### Structures de données (évolution indicative)
```c
#define MAX_CLIENTS 20
#define TAILLE_PSEUDO 32

typedef struct client {
    int fd;                          /* socket du client */
    struct sockaddr_in adresse;      /* IP + port du client */
    char pseudo[TAILLE_PSEUDO];      /* vide tant que /nick n'a pas été fait (V3) */
    time_t date_connexion;           /* pour /whois (V3) */
    pthread_mutex_t verrou_envoi;    /* empêche 2 threads d'écrire en même temps sur ce fd */
} client_t;
```
Tableau global `client_t *clients[MAX_CLIENTS]` + `int nb_clients`, protégés par `pthread_mutex_t mutex_clients`.
En V4 : tableau de salons protégé par `mutex_salons` (ou le même mutex, plus simple et sans risque de deadlock — **préférer un seul mutex global** pour la liste des clients et des salons).

### Règles de concurrence (IMPORTANT)
1. **Toute lecture ou modification** de la liste des clients ou des salons se fait entre `pthread_mutex_lock` et `pthread_mutex_unlock`.
2. **Chaque chemin de sortie** d'une section verrouillée (y compris les `return` et `break` dans des `if`) doit déverrouiller. Préférer une seule sortie par fonction.
3. **Envoi vers un autre client** (broadcast, MP, salon, relais de fichier) : plusieurs threads peuvent écrire sur la même socket → chaque envoi de trame vers un client passe par son `verrou_envoi`, pour que les trames ne se mélangent jamais.
4. **Ordre des verrous** pour éviter les deadlocks : toujours `mutex_clients` d'abord, puis `verrou_envoi` d'un client. Jamais l'inverse.
5. Un client ne doit jamais être `free()` pendant qu'un autre thread l'utilise : le retrait du tableau se fait sous `mutex_clients`, et tout envoi vers un autre client se fait **en tenant `mutex_clients`** (on accepte que cela sérialise les envois, c'est suffisant pour 20 clients).
6. Pas de variable globale modifiée sans mutex. Pas d'utilisation de fonctions non thread-safe (`strtok` → `strtok_r`, `localtime` → `localtime_r`, `inet_ntoa` → `inet_ntop`).

### Client
- Thread principal : `fgets()` sur le clavier → retire le `\n` → envoie en `T_TEXTE` (ignore les lignes vides).
- Thread de réception : boucle `recevoir_trame()` → affiche. S'il reçoit `T_BYE` ou si la connexion est fermée, il affiche `Connection terminated`, et le programme se termine proprement.
- À la fin : `close()` de la socket, `free()` de tout ce qui a été alloué, `pthread_join` ou terminaison propre.
- Affichage : invite `%> ` comme dans le sujet ; quand un message arrive pendant la saisie, l'afficher sur une nouvelle ligne puis réafficher l'invite.

---

## 6. Les 5 versions

### Version 1 — Modèle client/serveur (écho, 1 client)
Exigences du sujet :
- Le client se connecte en TCP au serveur (port + adresse IPv4).
- Le serveur crée une socket d'écoute, accepte la connexion et gère les données entrantes.
- Le client lit une chaîne au clavier, l'envoie, et reçoit **la même chaîne** en retour.
- Si le client envoie `/quit`, la connexion se coupe, **toutes les sockets sont fermées et la mémoire libérée**.

Précisions :
- Dans cette version, le serveur traite un client à la fois (pas encore de thread côté serveur). Après le départ d'un client, il peut en accepter un autre.
- Le client peut rester simple (envoi puis attente de la réponse) — les 2 threads côté client arrivent en V2.
- Mettre en place `protocole.h/.c` dès maintenant.

Test : `tests/test_v1.sh` — lance le serveur, envoie quelques lignes puis `/quit` via un client, vérifie l'écho.

---

### Version 2 — Serveur multi-client
Exigences du sujet :
- Lancement : `./client port_number IP_address`.
- Le client peut envoyer et recevoir des données.
- `/quit` coupe la connexion ; sockets fermées, mémoire libérée. Affichage **exact** attendu côté client :
  ```
  %> /quit
  %> [Server] : You will be terminated
  %> Connection terminated
  ```
- Le serveur répète la chaîne **uniquement au client émetteur** :
  ```
  % > Hello World!
  % > [Server] : Hello World!
  ```
- Le serveur maintient **20 connexions simultanées, pas plus**, sans crasher. Le **21ᵉ** client est refusé avec le message exact :
  ```
  Server cannot accept incoming connections anymore. Try again later.
  ```
  (le client affiche ce message puis se termine proprement).

Travail :
- Introduire un thread par client côté serveur, le tableau de clients et `mutex_clients`.
- Introduire les 2 threads côté client.
- Gérer la **déconnexion brutale** d'un client (Ctrl+C) : le serveur libère la place sans planter. Gérer aussi la fermeture du serveur côté client (message + sortie propre).
- Le serveur affiche dans sa console un journal : connexion (IP:port), déconnexion, nombre de clients connectés.

Test : `tests/test_v2.sh` — lance 20 clients en arrière-plan, vérifie qu'ils sont acceptés, lance un 21ᵉ et vérifie le message de refus ; ferme un client puis vérifie qu'un nouveau peut se connecter.

---

### Version 3 — Gestion des utilisateurs
Exigences du sujet :
- Une fois connecté, le client **doit** s'identifier par un pseudo : `/nick <pseudo>`.
- Le serveur gère plusieurs utilisateurs et connexions et stocke pour chacun : pseudo, numéro de socket (fd), infos de connexion.
- Le serveur prend en compte un nouvel utilisateur **et les changements de pseudo**.
- `/who` : liste des utilisateurs connectés.
- `/whois <pseudo>` : informations sur un utilisateur (pseudo, IP, port, date/heure de connexion, durée de connexion).
- Le serveur **garde son rôle d'écho** pour les messages qui ne sont pas des commandes.

Règles :
- Tant qu'un client n'a pas de pseudo, toute autre commande ou message (sauf `/nick`, `/quit`, `/help`) reçoit : `[Server] : Please identify yourself with /nick <pseudo>`. À la connexion, le serveur envoie un message d'accueil invitant à faire `/nick`.
- Pseudo valide : 1 à 31 caractères, lettres/chiffres/`_`/`-`, sans espace. **Unique** (comparaison insensible à la casse) ; sinon erreur « pseudo déjà utilisé ».
- Changement de pseudo : confirmation au client (`[Server] : You are now known as <nouveau>`).
- `/who` sur un serveur vide de pseudos ou `/whois` sur un pseudo inconnu → message d'erreur clair.
- Commande inconnue → `[Server] : Unknown command. Type /help`.
- `/help` liste les commandes disponibles dans la version.

Test : `tests/test_v3.sh`.

---

### Version 4 — Application de chat
Exigences du sujet :
- **Broadcast** : un utilisateur envoie un message à tous les autres.
- Un message envoyé **n'est pas retransmis à l'expéditeur**.
- **Unicast** : message privé à un autre utilisateur.
- Un utilisateur peut **créer un salon** ; erreur si le salon existe déjà.
- Un utilisateur peut **rejoindre et quitter** un salon.
- Le serveur **détruit le salon** quand son dernier occupant le quitte (y compris s'il se déconnecte).
- Un message de salon n'est transmis **qu'aux membres du salon** (multicast).

Commandes :
| Commande | Effet | Affichage chez le(s) destinataire(s) |
|---|---|---|
| `/all <message>` | broadcast à tous sauf l'expéditeur | `[alice] : message` |
| `/msg <pseudo> <message>` | message privé | `[MP de alice] : message` |
| `/create <salon>` | crée le salon et y fait entrer le créateur | — |
| `/join <salon>` | rejoint un salon existant | `[#salon] alice a rejoint le salon` (aux autres membres) |
| `/leave <salon>` | quitte le salon (destruction si vide) | `[#salon] alice a quitté le salon` |
| `/rooms` | liste des salons et nombre de membres | — |
| `/send <salon> <message>` | message dans un salon (il faut en être membre) | `[#salon][alice] : message` |
| texte sans `/` | toujours **l'écho** (rôle répétitif conservé) | `[Server] : texte` |

Règles :
- Un utilisateur peut être dans plusieurs salons. Limites : `MAX_SALONS = 20`, nom de salon 1–31 caractères sans espace.
- Erreurs claires : salon existant, salon inexistant, pas membre du salon, pseudo destinataire inconnu, message privé à soi-même, message vide.
- Les salons sont alloués dynamiquement (`malloc`) et libérés (`free`) à leur destruction.
- Le serveur journalise création/destruction de salons.

Test : `tests/test_v4.sh` — 3 clients (alice, bob, carol) : vérifier broadcast (l'expéditeur ne reçoit rien), MP (seul le destinataire reçoit), salon (carol hors salon ne reçoit rien), création en double refusée, destruction du salon après le départ du dernier membre.

---

### Version 5 — Transfert de fichiers (relayé par le serveur)
Exigences du sujet :
- Un utilisateur (émetteur) peut envoyer un fichier à un autre (récepteur).
- Le **récepteur doit donner son approbation**.
- L'émetteur **et** le récepteur reçoivent la **confirmation** que l'envoi s'est bien déroulé.

Commandes :
- `/sendfile <pseudo> <chemin_du_fichier>` (côté émetteur)
- `/accept <pseudo_emetteur>` et `/refuse <pseudo_emetteur>` (côté récepteur)

Déroulement :
1. Le client émetteur vérifie que le fichier existe et est lisible (`stat`), puis envoie `T_FICHIER_OFFRE` (destinataire, nom de fichier **sans le chemin** via `basename`, taille en octets).
2. Le serveur vérifie que le destinataire existe, enregistre le transfert en attente, et transmet l'offre au récepteur : `[Server] : alice veut vous envoyer "photo.jpg" (245 Ko). Tapez /accept alice ou /refuse alice`.
3. Refus → le serveur prévient l'émetteur ; transfert annulé.
4. Acceptation → le serveur prévient l'émetteur, qui envoie le fichier en trames `T_FICHIER_DONNEES` de 4096 octets max, puis `T_FICHIER_FIN`. Le serveur **relaie chaque trame** au récepteur (avec le `verrou_envoi` du récepteur).
5. Le récepteur écrit le fichier dans un dossier `recus/` (créé si besoin ; si le nom existe déjà, ajouter un suffixe `_1`, `_2`…). À la fin, il vérifie que le nombre d'octets reçus = taille annoncée, puis envoie `T_FICHIER_CONFIRME`.
6. Le serveur transmet la confirmation : les **deux** voient `Transfert de "photo.jpg" terminé avec succès (245 Ko)`.

Règles :
- Fichiers binaires : ouvrir en `"rb"` / `"wb"` (ou `open`/`read`/`write`), ne jamais traiter les données comme des chaînes.
- Un seul transfert en attente par couple émetteur/récepteur. Taille maximale conseillée : 100 Mo.
- Si l'émetteur ou le récepteur se déconnecte pendant le transfert : annuler, prévenir l'autre, supprimer le fichier partiel, libérer la mémoire.
- Afficher une progression simple côté émetteur (ex. pourcentage tous les 10 %).
- Le chat doit continuer à fonctionner pour les autres clients pendant un transfert.

Test : `tests/test_v5.sh` — transférer un fichier texte et un fichier binaire (ex. une image ou `head -c 5M /dev/urandom`), comparer avec `cmp` ou `md5sum`, tester le refus.

---

## 7. Qualité et vérification (à chaque version)

- Compilation sans warning : `make clean && make`.
- **Fuites mémoire** : lancer le serveur sous `valgrind --leak-check=full ./serveur 8080`, faire un scénario de test, arrêter le serveur proprement, et vérifier qu'il n'y a pas de fuite liée à notre code. Prévoir un arrêt propre du serveur sur `Ctrl+C` (gestion de `SIGINT` : fermer la socket d'écoute et les sockets clients, libérer la mémoire).
- Optionnel : compiler une fois avec `-fsanitize=thread` pour détecter les accès concurrents non protégés.
- Exécuter le script `tests/test_vN.sh` et montrer le résultat à Adama.
- Tester toujours aussi les cas d'erreur : mauvais arguments, serveur non démarré, client qui coupe brutalement, commande mal formée.

---

## 8. Git et GitHub

- Initialiser le dépôt Git au début (`git init`, branche `main`), avec un `.gitignore` (exécutables `serveur` et `client`, `*.o`, dossiers `recus/`, archives).
- **Commits fréquents et clairs**, en français, préfixés par la version : `v2: ajout du thread par client`, `v4: destruction du salon vide`…
- À la validation de chaque version par Adama : créer un **tag annoté** `v1.0`, `v2.0`… (`git tag -a v2.0 -m "Version 2 : serveur multi-client"`).
- GitHub : **demander à Adama** le nom du dépôt et s'il le veut public ou privé **avant** de le créer ou de pousser. Utiliser `gh` s'il est installé et connecté (`gh auth status`), sinon donner à Adama les commandes à exécuter. Pousser aussi les tags (`git push --tags`).
- Ne jamais commiter de fichiers personnels, de mots de passe ou de jetons.

---

## 9. README.md (vitrine GitHub)

Le dépôt sert aussi à valoriser le profil GitHub d'Adama. Le `README.md` racine doit être soigné :
- Titre + une phrase d'accroche (ex. « Application de chat multi-utilisateurs en C : sockets TCP, threads POSIX, salons et transfert de fichiers »).
- Badges simples (langage C, Linux, licence).
- Fonctionnalités principales (liste à puces).
- Une démonstration : bloc de terminal montrant une session (connexion, `/nick`, `/all`, salon, transfert).
- Schéma de l'architecture (en ASCII ou Mermaid) : clients ↔ serveur, thread par client, mutex.
- Description du protocole (format de trame).
- Tableau des commandes.
- Installation / compilation / lancement.
- Tableau « Évolution du projet » : V1 → V5 avec un lien vers chaque dossier.
- Section « Auteurs » : Sawadogo Adama (GitHub : Add6523), travail individuel, contexte (projet de Programmation système, L3 Informatique, UTS).
- Le README de chaque dossier de version reste court : ce qu'apporte la version et comment la tester.

---

## 10. Documentation à rendre (`docs/`)

À rédiger **à la fin du projet** (après la V5), en Markdown ; Adama les retouchera ensuite.

### `docs/rapport.md`
1. Page de garde : titre, cours, enseignant, année universitaire, **auteur (Sawadogo Adama, travail individuel) avec son adresse e-mail** (laisser des champs `[À COMPLÉTER]`).
2. Introduction : objectif du projet.
3. Choix techniques et justification (TCP, threads + mutex, protocole à en-tête, relais des fichiers par le serveur).
4. Architecture : schéma, structures de données, rôle de chaque fichier source.
5. Présentation version par version : ce qui a été fait, difficultés rencontrées, solutions.
6. Protocole de communication détaillé.
7. Tests réalisés et résultats (y compris valgrind).
8. Limites et améliorations possibles (ex. chiffrement TLS, persistance, interface graphique, `select()/epoll`).
9. Conclusion.

### `docs/manuel_utilisation.md`
Prérequis, compilation (`make`), lancement du serveur et des clients, liste complète des commandes avec exemples, scénario pas à pas (connexion → pseudo → salon → transfert), messages d'erreur fréquents et solutions.

### Conversion
Proposer une conversion en `.docx` et `.pdf` avec `pandoc` (l'installer avec `sudo apt install pandoc` si absent — demander à Adama avant tout `sudo`).

---

## 11. Livrable final

Le sujet exige une archive nommée `Travail_groupe_<NOM1>_<NOM2>.zip` (travail individuel : `Travail_groupe_SAWADOGO_Adama.zip`) contenant : le rapport, **les codes sources commentés, les exécutables**, le manuel d'utilisation, et tout fichier relatif au projet.

`livrable/creer_archive.sh` doit :
1. compiler chaque version (`make` dans `v1/` … `v5/`) ;
2. copier sources + exécutables de chaque version, `docs/` (versions `.pdf`/`.docx` si disponibles) et `README.md` dans un dossier temporaire ;
3. produire `Travail_groupe_<NOM>_<PRENOM>.zip` (par défaut `Travail_groupe_SAWADOGO_Adama.zip`) ;
4. ne pas inclure `.git`, les fichiers `.o` ni les dossiers `recus/`.

---

## 12. Rappels de méthode pour Claude Code

- Avancer **une version à la fois**, dans l'ordre. Annoncer le plan de la version avant de coder.
- Après chaque version : compiler, tester, vérifier valgrind, commit, puis **s'arrêter** pour l'explication et la validation d'Adama (voir §1). Le tag n'est posé qu'après sa validation.
- Si une exigence du sujet est ambiguë, choisir l'interprétation la plus simple, la documenter dans le README de la version et la signaler à Adama.
- Respecter **à la lettre** les messages imposés par le sujet (`[Server] : You will be terminated`, `Connection terminated`, `Server cannot accept incoming connections anymore. Try again later.`).
- Garder le code compréhensible par un étudiant de L3 : fonctions courtes, noms explicites, commentaires utiles.
