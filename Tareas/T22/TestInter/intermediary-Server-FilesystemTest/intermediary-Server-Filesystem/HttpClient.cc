
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
 #include <cstring>
 #include "Socket.h"
 
 #define BUFSIZE 2048
 
 int main() {
     while (true) {
         std::cout << "Elija una opción:" << std::endl;
         std::cout << std::endl << "1. GET figura\n2. POST figura\n3. LISTAR\n4. Salir\n> ";
 
         std::string opcion;
         std::getline(std::cin, opcion);
 
         if (opcion == "4") break;
 
         Socket s('s');
         // We are using localhost for simplicity, but you can change this to your machine's IP if needed
         s.MakeConnection("127.0.0.1", "8080"); // -> Use your own IP and port
 
         std::string request;
 
         if (opcion == "1") {
             std::string figura;
             std::cout << "Nombre figura: ";
             std::getline(std::cin, figura);
 
             request =
                 "GET /figura/" + figura + " HTTP/1.1\r\n"
                 "Host: test\r\n\r\n";
         }
         else if (opcion == "2") {
             std::string figura;
             std::cout << "Nombre figura: ";
             std::getline(std::cin, figura);
 
             std::string contenido, linea;
 
             std::cout << "Contenido (END para terminar):" << std::endl;
             while (true) {
                 std::getline(std::cin, linea);
                 if (linea == "END") break;
                 contenido += linea + "\n";
             }
 
             request =
                 "POST /figura/" + figura + " HTTP/1.1\r\n"
                 "Content-Type: text/plain\r\n\r\n" +
                 contenido;
         }
         else if (opcion == "3") {
             request =
                 "GET /figuras HTTP/1.1\r\n"
                 "Host: test\r\n\r\n";
         }
         else {
             std::cout << "Opcion invalida" << std::endl;
             continue;
         }
 
         s.Write(request.c_str());
 
         char buffer[BUFSIZE];
         int bytes;
 
         while ((bytes = s.Read(buffer, BUFSIZE - 1)) > 0) {
             buffer[bytes] = '\0';
             std::cout << buffer;
         }
 
         std::cout << std::endl << "--------------------------------" << std::endl;
 
         s.Close();
     }
 
     return 0;
 }