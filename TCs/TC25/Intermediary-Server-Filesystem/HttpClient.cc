
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
         std::cout << std::endl << "1. GET figura\n2. POST figura\n3. LISTAR\n4. ELIMINAR figura\n5. Salir\n> ";
 
         std::string opcion;
         std::getline(std::cin, opcion);
 
         if (opcion == "5") break;
 
         Socket s('s');
         // We are using localhost for simplicity, but you can change this to your machine's IP if needed
         s.MakeConnection("127.0.0.1", "8080"); //Aquí cambiamos la ip por la que tenga el inter y el puerto 8080 se queda igual
 
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
         else if (opcion == "4") {
             std::string figura;
             std::cout << "Nombre figura: ";
             std::getline(std::cin, figura);

             request =
                 "DELETE /figura/" + figura + " HTTP/1.1\r\n"
                 "Host: test\r\n\r\n";
         }
         else {
             std::cout << "Opcion invalida" << std::endl;
             continue;
         }
 
         s.Write(request.c_str());

        std::string raw;
        char buffer[BUFSIZE];
        int bytes;
        while ((bytes = s.Read(buffer, BUFSIZE - 1)) > 0) {
            buffer[bytes] = '\0';
            raw += buffer;
        }
        s.Close();

        size_t statusEnd = raw.find("\r\n");
        std::string statusLine = (statusEnd != std::string::npos) ? raw.substr(0, statusEnd) : "";
        bool isOk = statusLine.find("200") != std::string::npos;

        size_t start = raw.find("<result>");
        size_t end   = raw.find("</result>");
        if (start != std::string::npos && end != std::string::npos) {
            std::string result = raw.substr(start + 8, end - start - 8);
            if (!isOk) std::cout << statusLine << std::endl;
            std::cout << result;
        } else {
            std::cout << raw;
        }

        std::cout << std::endl << "--------------------------------" << std::endl;
     }
 
     return 0;
 }