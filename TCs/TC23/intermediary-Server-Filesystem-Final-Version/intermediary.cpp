#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <chrono>
#include <atomic>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "Socket.h"
#include "SSLSocket.h"
#include "Logger.h"

//constanres
#define HTTP_PORT        8080    //Clientes HTTP / NachOS
#define DISCOVERY_PORT   2026    //UDP entre islas(protocolo grupal)
#define BUFSIZE          4096
#define DISCOVERY_INTERVAL 5     //segundos entre broadcasts

//Red privada - 172.16.123.64/28 // BC - 172.16.123.79
static const std::string MY_BC       = "172.16.123.79";
//Direcciones BC de las demas de la isla
static const std::vector<std::string> OTHER_BC = {
    "172.16.123.31",   //Isla 1
    "172.16.123.47",   //Isla 2
    "172.16.123.63",   //Isla 3
    "172.16.123.95",   //Isla 5
    "172.16.123.111"   //Isla 6
};


//server propiuo, se puede cambiar con argumentos ./intermediary.out <ip> <puerto>
std::string SERVER_IP   = "127.0.0.1";
std::string SERVER_PORT = "1235";

 
//routeMap con la ip_origen y conjunto de nombres de figuras que ese fork conoce
//se construye el listado completo en P/R/ y se redirige un P/G/ al fork correcto
//"LOCAL" es la key para las figuras del servidor propio

static const std::string LOCAL_KEY = "LOCAL";
 
std::map<std::string, std::set<std::string>> routeMap;
std::mutex mapMutex;
std::set<std::string> localFigures;
std::mutex localMutex;
 
std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, delim)) {
        if (!tok.empty()) tokens.push_back(tok);
    }
    return tokens;
}


std::string getMethod(const std::string& req) {
    return req.substr(0, req.find(' '));
}


std::string getPath(const std::string& req) {
    size_t start = req.find(' ') + 1;
    size_t end   = req.find(" HTTP");
    return req.substr(start, end - start);
}


std::string getBody(const std::string& req) {
    size_t pos = req.find("\r\n\r\n");
    return (pos == std::string::npos) ? "" : req.substr(pos + 4);
}


std::string getFigureName(const std::string& path) {
    return path.substr(path.find_last_of('/') + 1);
}


std::string pipesToNewlines(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c == '|') c = '\n';
    return r;
}

//se construye una respuesta http basica con body HTML
//y el cliente NachOS busca <result>...</result> dentro del body para extraerla

std::string buildResponse(int status, const std::string& data) {
    std::string statusLine;
    if      (status == 200) statusLine = "HTTP/1.1 200 OK";
    else if (status == 404) statusLine = "HTTP/1.1 404 Not Found";
    else if (status == 400) statusLine = "HTTP/1.1 400 Bad Request";
    else                    statusLine = "HTTP/1.1 500 Internal Server Error";
 
    std::string body =
        "<html><head><title>Fork Isla4</title>"
        "<style>body{font-family:monospace;padding:20px}"
        "pre{background:#f4f4f4;padding:12px;border-radius:6px}"
        "</style></head><body>"
        "<result>" + data + "</result>"
        "</body></html>\n";
 
    return statusLine + "\r\n"
           "Content-Type: text/html; charset=utf-8\r\n\r\n" + body;
}

std::string queryLocalServer(const std::string& proto) {
    SSLSocket s('s');
    try {
        s.MakeConnection(SERVER_IP.c_str(), SERVER_PORT.c_str());
        s.InitClient();
        s.DoSSLConnect();
        s.Write(proto.c_str());
        char buf[BUFSIZE] = {0};
        int n = s.Read(buf, BUFSIZE - 1);
        buf[n] = '\0';
        return std::string(buf);
    } catch (...) {
        return "ERROR";
    }
}

//se actualiza localFigures y routeMap[LOCAL_KEY].
bool refreshLocalFigures() {
    std::string resp = queryLocalServer("P/R/");
    if (resp.find("ERROR") != std::string::npos || resp.size() <= 4) return false;
    // resp = P/D/<nombre1>\n<nombre2>...
    std::string data = resp.substr(4);
    auto names = split(data, '\n');
    std::lock_guard<std::mutex> lk(localMutex);
    std::lock_guard<std::mutex> mk(mapMutex);
    localFigures.clear();
    routeMap[LOCAL_KEY].clear();
 
    for (auto& n : names) {
        if (!n.empty()) {
            localFigures.insert(n);
            routeMap[LOCAL_KEY].insert(n);
        }
    }
 
    log("[LOCAL] Figuras propias: " + std::to_string(localFigures.size()));
    return !localFigures.empty();
}

// como necesitamos el socket antes y se puso después, se declara desde acá
int createUdpSocket(int port, bool reuseAddr);

//SE cambió toda la parte tcp a udp y la comunicación con el servidor de figuras local sigue
//siendo TCP/SSL (queryLocalServer), eso no cambia.
#define FORK_UDP_TIMEOUT_MS  2000// espera para la respuesta de otro fork

//se pide la figura a un fork externo por udp, se crea el socket y se espera la respuesta con el timeout
std::string getFigureFromFork(const std::string& ip, const std::string& figureName) {
    std::cout << "[BUSCANDO FORK] Solicitando '" << figureName
              << "' a " << ip << ":" << DISCOVERY_PORT << " (UDP)..." << std::endl;
    log("[FORK-UDP] Solicitando '" + figureName + "' a " + ip);

    int fd = createUdpSocket(0, true);   // socket efímero, sin bind fijo
    if (fd < 0) {
        std::cout << "[ERROR] No se pudo crear socket UDP para contactar " << ip << std::endl;
        return "";
    }

    // Timeout de recepción: si el fork no responde a tiempo, se asume caído
    struct timeval tv{};
    tv.tv_sec  = FORK_UDP_TIMEOUT_MS / 1000;
    tv.tv_usec = (FORK_UDP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(DISCOVERY_PORT);
    inet_pton(AF_INET, ip.c_str(), &dest.sin_addr);
    std::string msg = "P/G/" + figureName;
    sendto(fd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&dest, sizeof(dest));

    char buf[BUFSIZE] = {0};
    struct sockaddr_in from{};
    socklen_t fromLen = sizeof(from);
    int n = recvfrom(fd, buf, BUFSIZE - 1, 0, (struct sockaddr*)&from, &fromLen);
    close(fd);
    if (n <= 0) {
        std::cout << "[FORK NO RESPONDE] " << ip << " no contestó a tiempo, "
                  << "eliminando del mapa de rutas." << std::endl;
        log("[ROUTE] Fork " + ip + " no responde, eliminando del mapa");
        {
            std::lock_guard<std::mutex> lk(mapMutex);
            routeMap.erase(ip);
        }
        return "";
    }

    buf[n] = '\0';
    std::string resp(buf);

    if (resp.size() > 4 && resp.rfind("P/D/", 0) == 0) {
        std::cout << "[FORK ENCONTRADO] " << ip << " respondió con la figura '"
                  << figureName << "'." << std::endl;
        log("[FORK-UDP] Respuesta recibida de " + ip);
        return resp.substr(4);
    }

    std::cout << "[FORK ENCONTRADO] " << ip << " respondió, pero sin la figura." << std::endl;
    return "";
}

//se crea el socket para broadcast y hace bind en el puerto seleccionado
int createUdpSocket(int port, bool reuseAddr = true) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;
    int opt = 1;
    if (reuseAddr)
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    if (port > 0) {
        struct sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(fd);
            return -1;
        }
    }
    return fd;
}

std::string localFiguresCSV() {
    std::lock_guard<std::mutex> lk(localMutex);
    std::string csv;
    for (auto& f : localFigures) csv += f + ",";
    if (!csv.empty() && csv.back() == ',') csv.pop_back();
    return csv;
}
 
//hilo receptor UDP escuchando en el broadcast propio 172.16.123.79:2026
//Si recibe P/F/ responde con P/D/<figuras_propias>
//Si recibe P/D/ actualiza routeMap con las figuras de la ip de origen
void udpListenerThread() {
    int fd = createUdpSocket(DISCOVERY_PORT);
    if (fd < 0) {
        log("[UDP-LISTEN] ERROR: no pudo crear socket en puerto " +
            std::to_string(DISCOVERY_PORT));
        return;
    }
    log("[UDP-LISTEN] Escuchando en " + MY_BC + ":" +
        std::to_string(DISCOVERY_PORT));
 
    while (true) {
        char buf[BUFSIZE] = {0};
        struct sockaddr_in sender{};
        socklen_t senderLen = sizeof(sender);
        int n = recvfrom(fd, buf, BUFSIZE - 1, 0,
                         (struct sockaddr*)&sender, &senderLen);
        if (n <= 0) continue;
        buf[n] = '\0';
        std::string msg(buf);
        char senderIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender.sin_addr, senderIP, sizeof(senderIP));
        std::string srcIP(senderIP);
        log("[UDP-LISTEN] Mensaje de " + srcIP + ": " + msg);
 
        if (msg.rfind("P/F/", 0) == 0) {
            std::cout << "[BUSCANDO FORK] " << srcIP << " esta buscando forks "
                      << "en la red, respondiendo con figuras propias..." << std::endl;
            std::string csv = localFiguresCSV();
            if (csv.empty()) {
                continue;
            }
            std::string reply = "P/D/" + csv;
            struct sockaddr_in dest{};
            dest.sin_family      = AF_INET;
            dest.sin_port        = sender.sin_port; // reply to sender's actual port (ephemeral or 2026)
            dest.sin_addr        = sender.sin_addr;
 
            sendto(fd, reply.c_str(), reply.size(), 0,
                   (struct sockaddr*)&dest, sizeof(dest));

            std::cout << "[FORK ENCONTRADO] Respondido a " << srcIP
                       << " con " << localFigures.size() << " figura(s)." << std::endl;
            log("[UDP-LISTEN] Respondido P/D/ a " + srcIP);

        } else if (msg.rfind("P/G/", 0) == 0) {
            std::string figName = msg.substr(4);
            std::cout << "[BUSCANDO FORK] " << srcIP << " solicita la figura '"
                      << figName << "' via UDP." << std::endl;

            std::thread([fd, sender, figName, srcIP]() {
                std::string resp = queryLocalServer("P/G/" + figName);
                std::string reply;
                if (resp.find("ERROR") != std::string::npos || resp.size() <= 4) {
                    reply = "P/D/ERROR_404";
                } else {
                    reply = resp;//ya debe de venir como "P/D/<contenido>"
                }

                struct sockaddr_in dest{};
                dest.sin_family = AF_INET;
                dest.sin_port   = sender.sin_port; // reply to sender's actual port
                dest.sin_addr   = sender.sin_addr;

                sendto(fd, reply.c_str(), reply.size(), 0,
                       (struct sockaddr*)&dest, sizeof(dest));

                std::cout << "[FORK ENCONTRADO] Respondida solicitud de '"
                          << figName << "' a " << srcIP << "." << std::endl;
                log("[UDP-LISTEN] Respondido P/G/" + figName + " a " + srcIP);
            }).detach();

        } else if (msg.rfind("P/D/", 0) == 0) {
            std::cout << "[FORK ENCONTRADO] " << srcIP << " respondió con su "
                      << "listado de figuras, actualizando tabla de rutas." << std::endl;
            std::string data = msg.substr(4);
            auto names = split(data, ',');
 
            std::lock_guard<std::mutex> lk(mapMutex);
            routeMap[srcIP].clear();
            for (auto& name : names) {
                if (!name.empty()) routeMap[srcIP].insert(name);
            }
            log("[UDP-LISTEN] Mapa actualizado para " + srcIP +
                " (" + std::to_string(routeMap[srcIP].size()) + " figuras)");
        }
    }
    close(fd);
}
 
//Ahora se envía y espera la respuesta en el mismo socket, ne se usa el efímero porque se estaba
//perdiendo y no llegaba al 2026, básicamente ya no depende el udplistener para recibir la respuesta
void discoveryThread(const std::string& bcAddr) {
    log("[DISCOVERY] Hilo listo para isla BC=" + bcAddr);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(DISCOVERY_INTERVAL));
        {
            std::lock_guard<std::mutex> lk(localMutex);
            if (localFigures.empty()) continue;
        }

        int fd = createUdpSocket(0);
        if (fd < 0) {
            log("[DISCOVERY] ERROR creando socket para " + bcAddr);
            continue;
        }

        //se cierra el socket y se reintenta en el siguiente ciclo si el timeout se cumple
        struct timeval tv{};
        tv.tv_sec  = FORK_UDP_TIMEOUT_MS / 1000;
        tv.tv_usec = (FORK_UDP_TIMEOUT_MS % 1000) * 1000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        struct sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port   = htons(DISCOVERY_PORT);
        inet_pton(AF_INET, bcAddr.c_str(), &dest.sin_addr);

        std::string msg = "P/F/";
        std::cout << "[BUSCANDO FORK] Enviando broadcast a isla BC="
                  << bcAddr << "..." << std::endl;
        int sent = sendto(fd, msg.c_str(), msg.size(), 0,
                          (struct sockaddr*)&dest, sizeof(dest));
        if (sent < 0) {
            log("[DISCOVERY] Error enviando a " + bcAddr);
            close(fd);
            continue;
        }
        log("[DISCOVERY] P/F/ enviado a " + bcAddr);

        //se espera la respuesta P/D/ en este mismo socket
        char buf[BUFSIZE] = {0};
        struct sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        int n = recvfrom(fd, buf, BUFSIZE - 1, 0,
                         (struct sockaddr*)&from, &fromLen);
        close(fd);

        if (n <= 0) {
            std::cout << "[BUSCANDO FORK] Sin respuesta de isla BC="
                      << bcAddr << " (timeout)." << std::endl;
            continue;
        }

        buf[n] = '\0';
        std::string resp(buf);

        char fromIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, fromIP, sizeof(fromIP));
        std::string srcIP(fromIP);

        if (resp.size() <= 4 || resp.rfind("P/D/", 0) != 0) continue;

        std::cout << "[FORK ENCONTRADO] " << srcIP << " respondio con su "
                  << "listado de figuras, actualizando tabla de rutas." << std::endl;

        std::string data = resp.substr(4);
        auto names = split(data, ',');

        std::lock_guard<std::mutex> lk(mapMutex);
        routeMap[srcIP].clear();
        for (auto& name : names) {
            if (!name.empty()) routeMap[srcIP].insert(name);
        }
        log("[DISCOVERY] Mapa actualizado para " + srcIP +
            " (" + std::to_string(routeMap[srcIP].size()) + " figuras)");
    }
}

//se refresca localFigures de manera periuodica por si se llegaran a agregar figuras localmente
void localServerRefreshThread() {
    log("[LOCAL-REFRESH] Hilo iniciado");
    while (true) {
        bool ok = refreshLocalFigures();
        if (!ok) {
            log("[LOCAL-REFRESH] Sin figuras todavía, reintentando en " +
                std::to_string(DISCOVERY_INTERVAL) + "s");
        }
        std::this_thread::sleep_for(std::chrono::seconds(DISCOVERY_INTERVAL));
    }
}
  

//se busca en el routemap la ip de la figura solicitada 
std::string findFigureOwner(const std::string& name) {
    std::lock_guard<std::mutex> lk(mapMutex);
    for (auto& [ip, figs] : routeMap) {
        if (figs.count(name)) return ip;
    }
    return "";
}

//se devuelve el listado completo de figuras de todos los forks conocidos, a nuestro cliente 
std::string allFiguresList() {
    std::lock_guard<std::mutex> lk(mapMutex);
    std::string list;
    for (auto& [ip, figs] : routeMap) {
        for (auto& f : figs) list += f + "\n";
    }
    return list;
}
//se evitan duplicados en nombres de figuras
std::string resolveUniqueName(const std::string& name) {
    std::string resp = queryLocalServer("P/G/" + name);
    if (resp.find("ERROR") != std::string::npos || resp.empty()) return name;
    for (int i = 1; i <= 99; i++) {
        std::string candidate = name + "(" + std::to_string(i) + ")";
        resp = queryLocalServer("P/G/" + candidate);
        if (resp.find("ERROR") != std::string::npos || resp.empty()) return candidate;
    }
    return name + "(99)";
}
 

//atencion del cliente HTTP un hilo separado por cada conexion
void handleClient(VSocket* client) {
    char buffer[BUFSIZE] = {0};
    client->Read(buffer, BUFSIZE - 1);
    std::string request(buffer);
 
    std::string method = getMethod(request);
    std::string path   = getPath(request);
 
    log("[HTTP] " + method + " " + path);
    if (method == "GET" && (path == "/" || path == "/index.html")) {
        //tabla de rutas construida para mostrarse
        std::string routeTable;
        {
            std::lock_guard<std::mutex> lk(mapMutex);
            for (auto& [ip, figs] : routeMap) {
                std::string label = (ip == LOCAL_KEY) ? "LOCAL (servidor propio)" : "Fork " + ip;
                routeTable += "<tr><td>" + label + "</td><td>";
                for (auto& f : figs) routeTable += f + " ";
                routeTable += "</td></tr>";
            }
        }
        std::string html =
            "<!DOCTYPE html><html lang=\"es\"><head>"
            "<meta charset=\"UTF-8\"/>"
            "<title>Fork Isla 4</title>"
            "<style>body{font-family:monospace;padding:20px}"
            "table{border-collapse:collapse;width:100%}"
            "th,td{border:1px solid #ccc;padding:8px;text-align:left}"
            "th{background:#f0f0f0}"
            "input{padding:6px;width:300px;margin-right:4px}"
            "button{padding:6px 12px;cursor:pointer}"
            ".result{margin-top:12px;padding:10px;background:#f9f9f9;"
            "border:1px solid #ddd;white-space:pre-wrap;display:none}"
            "</style></head><body>"
            "<h1>Fork - Isla 4 (Grupo 3)</h1>"
            "<h2>Tabla de rutas</h2>"
            "<table><tr><th>Origen</th><th>Figuras</th></tr>"
            + routeTable +
            "</table>"
            "<h2>Obtener figura</h2>"
            "<input id=\"fn\" placeholder=\"Nombre de figura\">"
            "<button onclick=\"getFig()\">Obtener</button>"
            "<div class=\"result\" id=\"r\"></div>"
            "<h2>Listar figuras</h2>"
            "<button onclick=\"listFigs()\">Ver lista</button>"
            "<div class=\"result\" id=\"lr\"></div>"
            "<script>"
            "function parse(t){const m=t.match(/<result>(.*?)<\\/result>/s);return m?m[1].trim():t;}"
            "function show(id,t){const e=document.getElementById(id);e.textContent=parse(t);e.style.display='block';}"
            "async function getFig(){const n=document.getElementById('fn').value.trim();"
            "if(!n)return;const r=await fetch('/figura/'+n);show('r',await r.text());}"
            "async function listFigs(){const r=await fetch('/figuras');show('lr',await r.text());}"
            "</script></body></html>\n";
 
        std::string resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n" + html;
        client->Write(resp.c_str());
        client->Close();
        return;
    }
 
    //GET /figura/<nombre>
    if (method == "GET" && path.find("/figura/") == 0) {
        std::string name  = getFigureName(path);
        std::string owner = findFigureOwner(name);
 
        if (owner.empty()) {
            client->Write(buildResponse(404, "Figura no encontrada: " + name).c_str());
        } else if (owner == LOCAL_KEY) {
            //figura local, se consulta al servidor propio
            std::string resp = queryLocalServer("P/G/" + name);
            if (resp.find("ERROR") != std::string::npos || resp.size() <= 4) {
                {
                    std::lock_guard<std::mutex> lk(mapMutex);
                    routeMap[LOCAL_KEY].erase(name);
                }
                {
                    std::lock_guard<std::mutex> lk(localMutex);
                    localFigures.erase(name);
                }
                client->Write(buildResponse(404, "Figura no disponible: " + name).c_str());
            } else {
                std::string data = resp.substr(4);
                client->Write(buildResponse(200, data).c_str());
            }
        } else {
            //Figura ajena, se contacta al fork dueño por TCP
            std::string data = getFigureFromFork(owner, name);
            if (data.empty()) {
                client->Write(buildResponse(503, "Fork remoto no disponible").c_str());
            } else {
                client->Write(buildResponse(200, data).c_str());
            }
        }
        client->Close();
        return;
    }
 
    //GET /figuras  (listado completo)
    if (method == "GET" && path == "/figuras") {
        std::string list = allFiguresList();
        client->Write(buildResponse(200, list.empty() ? "Sin figuras disponibles" : list).c_str());
        client->Close();
        return;
    }
 
    //POST /figura/<nombre>
    if (method == "POST" && path.find("/figura/") == 0) {
        std::string name = resolveUniqueName(getFigureName(path));
        std::string body = pipesToNewlines(getBody(request));
        std::string proto = "P/W/" + name + "|" + body;
 
        log("[POST] Guardando figura: " + name);
 
        SSLSocket s('s');
        try {
            s.MakeConnection(SERVER_IP.c_str(), SERVER_PORT.c_str());
            s.InitClient();
            s.DoSSLConnect();
            s.Write(proto.c_str());
            char buf[BUFSIZE] = {0};
            int n = s.Read(buf, BUFSIZE - 1);
            buf[n] = '\0';
            std::string resp(buf);
 
            if (resp.find("ERROR") != std::string::npos) {
                client->Write(buildResponse(500, "Error al guardar").c_str());
            } else {
                // se va a actualizar el mapa local inmediatamente
                {
                    std::lock_guard<std::mutex> lk(mapMutex);
                    routeMap[LOCAL_KEY].insert(name);
                }
                {
                    std::lock_guard<std::mutex> lk(localMutex);
                    localFigures.insert(name);
                }
                client->Write(buildResponse(200, "Guardado como: " + name).c_str());
            }
        } catch (...) {
            client->Write(buildResponse(500, "Servidor de figuras no disponible").c_str());
        }
        client->Close();
        return;
    }
    client->Write(buildResponse(400, "Ruta no valida").c_str());
    client->Close();
}
 
int main(int argc, char* argv[]) {
    // ip y puerto en caso de argumentos
    if (argc >= 3) {
        SERVER_IP   = argv[1];
        SERVER_PORT = argv[2];
    }

    log("[SYSTEM] Fork Isla 4 iniciando");
    log("[SYSTEM] Servidor figuras: " + SERVER_IP + ":" + SERVER_PORT);
    log("[SYSTEM] BC propio: " + MY_BC + ":" + std::to_string(DISCOVERY_PORT));
 
    //Hilo para refrescar parte local
    std::thread(localServerRefreshThread).detach();
 
    //hilo para estar escuchando UDP en nuestro broadcast
    std::thread(udpListenerThread).detach();
 
    //hilos para las islas enviando P/F/ periodicamente
    for (const auto& bc : OTHER_BC) {
        std::thread(discoveryThread, bc).detach();
    }
 
    //atención de los clientes
    Socket httpServer('s');
    try {
        httpServer.Bind(HTTP_PORT);
        httpServer.MarkPassive(10);
        log("[HTTP] Servidor listo en puerto " + std::to_string(HTTP_PORT));
    } catch (...) {
        log("[CRITICAL] No pudo hacer bind en puerto " + std::to_string(HTTP_PORT));
        return 1;
    }
    while (true) {
        VSocket* client = nullptr;
        try {
            client = httpServer.AcceptConnection();
        } catch (...) {
            continue;
        }
        std::thread(handleClient, client).detach();
    }
    return 0;
}