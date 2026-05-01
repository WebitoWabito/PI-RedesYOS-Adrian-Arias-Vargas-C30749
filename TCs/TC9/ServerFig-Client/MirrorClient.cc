/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-i
  *  Grupos: 2 y 3
  *
  *   Socket client/server example
  *
  *   Deben determinar la dirección IP del equipo donde van a correr el servidor
  *   para hacer la conexión en ese punto (ip addr)
  *
 **/

#include <stdio.h>
#include <cstring>
#include "Socket.h"

#define PORT 2026
#define BUFSIZE 1024

int main(int argc, char **argv) {
    VSocket *s;
    char buffer[BUFSIZE];

    s = new Socket('s');
    memset(buffer, 0, BUFSIZE);

    s->Connect("127.0.0.1", PORT);

    if (argc > 1) {
        s->Write(argv[1]);
    } else {
        s->Write("P/R/dir");
    }

    s->Read(buffer, BUFSIZE);

    printf("%s\n", buffer);

    return 0;
}