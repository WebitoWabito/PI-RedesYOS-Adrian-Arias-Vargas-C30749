#include <iostream>
#include <thread>
#include <cstring>
#include "Socket.h"
#include <vector>
#include <string>
#include "Logger.h"
#include "SSLSocket.h"

#define PORT 8080
#define BUFSIZE 2048
#define INT_PORT 9000

// We are using localhost for simplicity, but you can change this to your machine's IP if needed
std::string SERVER_IP = "127.0.0.1"; // Change here if you change the client conection IP
std::string SERVER_PORT = "1235";

std::vector<std::string> intermediaries;

void handleIntermediary(VSocket* client) {
    char buffer[BUFSIZE] = {0};

    client->Read(buffer, BUFSIZE);
    std::string msg(buffer);

    log("[INTERMEDIARY] MSG recibido: " + msg);

    std::string senderAddr;

    if (msg.rfind("P/C/", 0) == 0) {
        senderAddr = msg.substr(4);
        intermediaries.push_back(senderAddr);

        log("[INTERMEDIARY] Registrado: " + senderAddr);

        client->Write("P/A/OK");
    }
    else if (msg.rfind("P/S/", 0) == 0) {
        senderAddr = msg.substr(4);

        log("[INTERMEDIARY] Sync recibido de: " + senderAddr);

        client->Write("P/A/SYNCED");
    }
    else {
        log("[ERROR] Mensaje inválido de intermediario");
        client->Write("P/A/ERROR");
        client->Close();
        return;
    }

    for (const auto& i : intermediaries) {

        if (i == senderAddr) continue;

        try {
            Socket s('s');

            size_t pos = i.find(":");
            std::string ip = i.substr(0, pos);
            std::string port = i.substr(pos + 1);

            log("[INTERMEDIARY] Notificando a: " + i);

            s.MakeConnection(ip.c_str(), port.c_str());

            std::string notify = "P/S/" + senderAddr;
            s.Write(notify.c_str());

            s.Close();
        } catch (...) {
            log("[WARNING] Falló notificar a: " + i);
        }
    }

    client->Close();
}

void registerToIntermediary(std::string ip, std::string port) {
    try {
        log("[INTERMEDIARY] Intentando registro en " + ip + ":" + port);

        Socket s('s');
        s.MakeConnection(ip.c_str(), port.c_str());

        std::string msg = "P/C/127.0.0.1:" + std::to_string(INT_PORT);
        s.Write(msg.c_str());

        char buffer[BUFSIZE] = {0};
        s.Read(buffer, BUFSIZE);

        log("[INTERMEDIARY] Respuesta registro: " + std::string(buffer));

        s.Close();
    } catch (...) {
        log("[ERROR] Failed register");
    }
}

void syncIntermediaries(std::string ip, std::string port) {
    try {
        log("[INTERMEDIARY] Sync enviado a " + ip + ":" + port);

        Socket s('s');
        s.MakeConnection(ip.c_str(), port.c_str());

        std::string msg = "P/S/127.0.0.1:" + std::to_string(INT_PORT);
        s.Write(msg.c_str());

        char buffer[BUFSIZE] = {0};
        s.Read(buffer, BUFSIZE);

        log("[INTERMEDIARY] Respuesta sync: " + std::string(buffer));

        s.Close();
    } catch (...) {
        log("[ERROR] Sync failed");
    }
}

std::string getMethod(const std::string& req) {
    return req.substr(0, req.find(" "));
}

std::string getPath(const std::string& req) {
    size_t start = req.find(" ") + 1;
    size_t end = req.find(" HTTP");
    return req.substr(start, end - start);
}

std::string getBody(const std::string& req) {
    size_t pos = req.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return req.substr(pos + 4);
}

std::string getFigureName(const std::string& path) {
    return path.substr(path.find_last_of("/") + 1);
}

void handleHTTP(VSocket* client) {
    char buffer[BUFSIZE] = {0};

    client->Read(buffer, BUFSIZE);
    std::string request(buffer);

    std::string method = getMethod(request);
    std::string path = getPath(request);

    log("[HTTP] Request: " + method + " " + path);

    std::string proto;

    if (method == "GET" && path.find("/figura/") == 0) {
        std::string name = getFigureName(path);
        proto = "P/G/" + name;
    }
    else if (method == "GET" && path == "/figuras") {
        proto = "P/R/";
    }
    else if (method == "POST" && path.find("/figura/") == 0) {
        std::string name = getFigureName(path);
        std::string body = getBody(request);
        proto = "P/W/" + name + "|" + body;
    }
    else {
        log("[ERROR] Ruta HTTP inválida");

        std::string resp =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Ruta no valida\n";

        client->Write(resp.c_str());
        client->Close();
        return;
    }

    log("[PROTO] Enviado al server: " + proto);

    SSLSocket s('s');  

    try {
        log("[SERVER] Conectando a " + SERVER_IP + ":" + SERVER_PORT);

        s.MakeConnection(SERVER_IP.c_str(), SERVER_PORT.c_str());

        s.InitClient();     
        s.DoSSLConnect();   

        s.Write(proto.c_str());

        char serverBuffer[BUFSIZE] = {0};
        int bytes = s.Read(serverBuffer, BUFSIZE - 1);
        serverBuffer[bytes] = '\0';

        std::string response(serverBuffer);

        log("[SERVER] Respuesta: " + response);

        std::string httpResponse;

        if (response.find("ERROR") != std::string::npos) {
            log("[ERROR] Server respondió error");

            httpResponse =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n\r\n"
                "Error en servidor\n";
        } else {
            std::string data = response.substr(4);

            httpResponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n\r\n" + data;
        }

        client->Write(httpResponse.c_str());

    } catch (...) {
        log("[CRITICAL] Fallo conexión con servidor");

        std::string resp =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Server unreachable\n";

        client->Write(resp.c_str());
    }

    client->Close();
}

int main() {
    Socket server('s');
    Socket intServer('s');

    server.Bind(PORT);
    server.MarkPassive(5);

    intServer.Bind(INT_PORT);
    intServer.MarkPassive(5);

    log("[SYSTEM] Intermediario iniciado en puerto " + std::to_string(PORT));

    std::thread httpLoop([&]() {
        while (true) {
            VSocket* client = server.AcceptConnection();
            std::thread(handleHTTP, client).detach();
        }
    });

    std::thread intLoop([&]() {
        while (true) {
            VSocket* c = intServer.AcceptConnection();
            std::thread(handleIntermediary, c).detach();
        }
    });

    httpLoop.detach();
    intLoop.detach();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}