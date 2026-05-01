/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-i
  *  Grupos: 2 y 3
  *
  *   Socket client/server example
  *
  * (Fedora version)
  *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "Socket.h"
#include "FS.h"

#define PORT 2026
#define BUFSIZE 512

using namespace std;

int main() {
    VSocket *s1, *s2;
    int childpid;
    char buffer[BUFSIZE];

    FileSystem fs;

    vector<string> perro = {
        "pieza 1p",
        "pieza 2p",
        "pieza 3p"
    };

    vector<string> gato = {
        "pieza 1g",
        "pieza 2g",
        "pieza 3g"
    };

    vector<string> caballo = {
        "pieza 1c",
        "pieza 2c",
        "pieza 3c"
    };
   
    fs.saveFigure("Perro", perro);
    fs.saveFigure("Gato", gato);
    fs.saveFigure("Caballo", caballo);

    s1 = new Socket('s');
    s1->Bind(PORT);
    s1->MarkPassive(5);

    for (;;) {
        s2 = s1->AcceptConnection();
        childpid = fork();

        if (childpid == 0) {
            s1->Close();

            memset(buffer, 0, BUFSIZE);
            s2->Read(buffer, BUFSIZE);

            string request(buffer);
            string response;

            if (request.substr(0, 2) != "P/") {
                response = "ERROR: Protocolo invalido";
            } else {
                char command = request[2];

                if (command == 'G') { //GET
                    string fig = request.substr(4);
                    string data = fs.readFigure(fig);
                    response = "P/D/" + data;
                }
                else if (command == 'R') { //REQUEST
                    string list = fs.listFigures();
                    response = "P/D/" + list;
                }
                else if (command == 'Q') {
                    response = "P/A/";
                }
                else {
                    response = "ERROR: Comando desconocido";
                }
            }

            s2->Write(response.c_str());
            exit(0);
        }

        s2->Close();
    }
}