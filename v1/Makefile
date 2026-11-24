# Makefile — Version 1 (client/serveur écho)
#
#   make              compile ./serveur et ./client
#   make clean        supprime les fichiers produits
#   make run-serveur  lance le serveur (port 8080 par défaut, ou PORT=xxxx)

CC      = gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -pthread -g -Iinclude
LDFLAGS = -pthread
PORT   ?= 8080

# Fichiers objets de chaque programme
OBJ_COMMUN  = src/protocole.o
OBJ_SERVEUR = src/serveur.o $(OBJ_COMMUN)
OBJ_CLIENT  = src/client.o $(OBJ_COMMUN)

.PHONY: all clean run-serveur

all: serveur client

serveur: $(OBJ_SERVEUR)
	$(CC) $(LDFLAGS) -o $@ $^

client: $(OBJ_CLIENT)
	$(CC) $(LDFLAGS) -o $@ $^

# Règle générique : chaque .c donne un .o (recompilé si protocole.h change)
src/%.o: src/%.c include/protocole.h
	$(CC) $(CFLAGS) -c -o $@ $<

run-serveur: serveur
	./serveur $(PORT)

clean:
	rm -f serveur client src/*.o
