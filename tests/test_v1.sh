#!/bin/bash
# test_v1.sh — Tests automatiques de la Version 1 (client/serveur écho).
#
# Usage : ./tests/test_v1.sh   (depuis la racine du projet ou depuis tests/)
#
# Vérifie : compilation sans warning, écho des lignes, /quit, enchaînement
# de deux clients, cas d'erreur (arguments, serveur absent), arrêt Ctrl+C.

RACINE="$(cd "$(dirname "$0")/.." && pwd)"
V1="$RACINE/v1"
PORT=9101
TMP="$(mktemp -d)"
REUSSIS=0
ECHECS=0

# verifier "<description>" <commande...> : affiche OK/ÉCHEC selon le code retour
verifier() {
    local description="$1"; shift
    if "$@"; then
        echo "  [OK]    $description"; REUSSIS=$((REUSSIS + 1))
    else
        echo "  [ÉCHEC] $description"; ECHECS=$((ECHECS + 1))
    fi
}

nettoyer() {
    [ -n "$PID_SERVEUR" ] && kill "$PID_SERVEUR" 2>/dev/null
    rm -rf "$TMP"
}
trap nettoyer EXIT

echo "== 1. Compilation =="
make -C "$V1" clean >/dev/null
make -C "$V1" > "$TMP/make.log" 2>&1
verifier "make réussit" test $? -eq 0
verifier "aucun warning" bash -c "! grep -qi warning '$TMP/make.log'"

echo "== 2. Démarrage du serveur =="
"$V1/serveur" "$PORT" > "$TMP/serveur.log" 2>&1 &
PID_SERVEUR=$!
sleep 0.5
verifier "le serveur tourne" kill -0 "$PID_SERVEUR"

echo "== 3. Premier client : écho puis /quit =="
printf 'Hello World!\nbonjour le serveur\n\n/quit\n' | timeout 5 "$V1/client" "$PORT" 127.0.0.1 > "$TMP/client1.log" 2>&1
verifier "le client se termine avec le code 0"  test $? -eq 0
verifier "écho de 'Hello World!'"               grep -q '\[Server\] : Hello World!$' "$TMP/client1.log"
verifier "écho de 'bonjour le serveur'"         grep -q '\[Server\] : bonjour le serveur$' "$TMP/client1.log"
verifier "réponse à /quit"                      grep -q '\[Server\] : You will be terminated' "$TMP/client1.log"
verifier "message 'Connection terminated'"      grep -q 'Connection terminated' "$TMP/client1.log"
verifier "la ligne vide n'a pas été envoyée"    test "$(grep -c '\[Server\]' "$TMP/client1.log")" -eq 3

echo "== 4. Deuxième client après le départ du premier =="
printf 'second client\n/quit\n' | timeout 5 "$V1/client" "$PORT" 127.0.0.1 > "$TMP/client2.log" 2>&1
verifier "le 2e client reçoit son écho"         grep -q '\[Server\] : second client$' "$TMP/client2.log"

echo "== 5. Fin d'entrée (Ctrl+D) traitée comme /quit =="
printf 'sans quit\n' | timeout 5 "$V1/client" "$PORT" 127.0.0.1 > "$TMP/client3.log" 2>&1
verifier "Ctrl+D termine proprement"            grep -q 'Connection terminated' "$TMP/client3.log"

echo "== 6. Cas d'erreur =="
"$V1/client" > /dev/null 2>&1
verifier "client sans argument -> échec"        test $? -ne 0
"$V1/client" 80 127.0.0.1 > /dev/null 2>&1
verifier "client port 80 refusé"                test $? -ne 0
"$V1/client" "$PORT" 999.1.1.1 > /dev/null 2>&1
verifier "client IP invalide refusée"           test $? -ne 0
"$V1/serveur" abc > /dev/null 2>&1
verifier "serveur port 'abc' refusé"            test $? -ne 0
echo "/quit" | timeout 5 "$V1/client" 9199 127.0.0.1 > /dev/null 2>&1
verifier "client sans serveur -> échec propre"  test $? -eq 1

echo "== 7. Client tué brutalement (kill -9) =="
sleep 5 | "$V1/client" "$PORT" 127.0.0.1 > /dev/null 2>&1 &
sleep 0.5
pkill -9 -x client
sleep 0.3
printf 'encore vivant\n/quit\n' | timeout 5 "$V1/client" "$PORT" 127.0.0.1 > "$TMP/client4.log" 2>&1
verifier "le serveur sert le client suivant"    grep -q '\[Server\] : encore vivant$' "$TMP/client4.log"

echo "== 8. Ctrl+C sur le serveur pendant qu'un client est connecté =="
# En V1 le client est séquentiel : il ne voit la coupure qu'au prochain envoi.
# On simule donc une ligne tapée environ 1,5 s après le Ctrl+C du serveur.
( sleep 2; echo "apres arret" ) | timeout 5 "$V1/client" "$PORT" 127.0.0.1 > "$TMP/client5.log" 2>&1 &
PID_CLIENT=$!
sleep 0.5
kill -INT "$PID_SERVEUR"
sleep 0.5
verifier "le serveur s'est arrêté immédiatement" bash -c "! kill -0 $PID_SERVEUR 2>/dev/null"
verifier "message d'arrêt propre"               grep -q 'Arrêt propre' "$TMP/serveur.log"
PID_SERVEUR=""
wait "$PID_CLIENT"
verifier "le client a vu la fin de connexion"  grep -q 'Connection terminated' "$TMP/client5.log"

echo
echo "Résultat : $REUSSIS test(s) réussi(s), $ECHECS échec(s)."
[ "$ECHECS" -eq 0 ]
