
/**
 *   UCR-ECCI
 *   CI-0123 Proyecto integrador de redes y sistemas operativos
 *
 *   Socket client/server example
 *
 *   Deben determinar la dirección IP del equipo donde van a correr el servidor
 *   para hacer la conexión en ese punto (ip addr)
 *
 **/

 #include <iostream>
#include <string>
#include "Socket.h"

#define BUFSIZE 512

int main() {
    VSocket* s = new Socket('s');

    s->MakeConnection("127.0.0.1", "1235");

    std::string input;

    std::cout << "Enter protocol command (P/...): ";
    std::getline(std::cin, input);

    s->Write(input.c_str());

    char buffer[BUFSIZE] = {0};
    s->Read(buffer, BUFSIZE);

    std::cout << "Server response: " << buffer << std::endl;

    s->Close();
}