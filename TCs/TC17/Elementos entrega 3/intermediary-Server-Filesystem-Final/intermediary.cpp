#include <iostream>
#include <thread>
#include <cstring>
#include "Socket.h"
#include <vector>
#include <string>
#include "Logger.h"
#include "SSLSocket.h"

#define PORT     8080
#define BUFSIZE  2048
#define INT_PORT 9000

std::string SERVER_IP   = "127.0.0.1";
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
            std::string ip   = i.substr(0, pos);
            std::string port = i.substr(pos + 1);
            s.MakeConnection(ip.c_str(), port.c_str());
            s.Write(("P/S/" + senderAddr).c_str());
            s.Close();
        } catch (...) {
            log("[WARNING] Falló notificar a: " + i);
        }
    }
    client->Close();
}


std::string getMethod(const std::string& req) {
    return req.substr(0, req.find(" "));
}

std::string getPath(const std::string& req) {
    size_t start = req.find(" ") + 1;
    size_t end   = req.find(" HTTP");
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

/**
 * This function builds a simple html reponse to displey the result in the browser or parse it in the nachOs client.
 */

/* Replace all pipe characters with newline for consistent storage format */
std::string pipesToNewlines(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        if (c == '|') c = '\n';
    }
    return result;
}

std::string buildResponse(int status, const std::string& data) {

    std::string statusLine;
    if (status == 200) statusLine = "HTTP/1.1 200 OK";
    else if (status == 404) statusLine = "HTTP/1.1 404 Not Found";
    else if (status == 400) statusLine = "HTTP/1.1 400 Bad Request";
    else statusLine = "HTTP/1.1 500 Internal Server Error";

    // Simple html to display the resul
    std::string body =
        "<html><head><title>NachOS FS</title>"
        "<style>body{font-family:monospace;padding:20px}"
        "pre{background:#f4f4f4;padding:12px;border-radius:6px}"
        "</style></head><body>"
        "<result>" + data + "</result>"
        "</body></html>\n";

    return
        statusLine + "\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "\r\n" +   // This is important because nachos looks for this separator to find the body and skip the headers
        body;
}


std::string queryServer(const std::string& proto) {
    SSLSocket s('s');
    try {
        s.MakeConnection(SERVER_IP.c_str(), SERVER_PORT.c_str());
        s.InitClient();
        s.DoSSLConnect();
        s.Write(proto.c_str());
        char buf[2048] = {0};
        int n = s.Read(buf, 2047);
        buf[n] = '\0';
        return std::string(buf);
    } catch (...) {
        return "ERROR";
    }
}

/**
 * We add this function to handle the case where a client tries to save a figure with a name that 
 * already exists on the server.
 */
std::string resolveUniqueName(const std::string& name) {
    // Veriry if the name already exists by querying the server with "P/G/name"
    std::string check = queryServer("P/G/" + name);
    if (check.find("ERROR") != std::string::npos || check.empty()) {
        return name;   // If doesn't exists, then use the original name
    }

    // Exists: Try name(1), name(2), ... until we find a free one
    for (int i = 1; i <= 99; i++) {
        std::string candidate = name + "(" + std::to_string(i) + ")";
        std::string res = queryServer("P/G/" + candidate);
        if (res.find("ERROR") != std::string::npos || res.empty()) {
            return candidate;   
        }
    }
    return name + "(99)"; 
}

void handleClient(VSocket* client) {

    char buffer[BUFSIZE] = {0};
    client->Read(buffer, BUFSIZE);
    std::string request(buffer);

    log("[CLIENT] Request recibida");

    std::string method = getMethod(request);
    std::string path   = getPath(request);

    log("[HTTP] " + method + " " + path);

    if (method == "GET" && (path == "/" || path == "/index.html")) {

        // This is an HTML page to interact in a better way
        std::string html =
            "<!DOCTYPE html><html lang=\"es\"><head>"
            "<meta charset=\"UTF-8\"/>"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/>"
            "<title>NachOS FS</title>"
            "<style>"
            "*{margin:0;padding:0;box-sizing:border-box}"
            ":root{--bg:#f5f5f7;--white:#fff;--text:#1d1d1f;--muted:#6e6e73;"
            "--border:#d2d2d7;--blue:#0071e3;--green:#1d8f45;"
            "--radius-sm:10px;--radius-lg:18px;}"
            "body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;"
            "background:var(--bg);color:var(--text);-webkit-font-smoothing:antialiased;"
            "min-height:100vh;}"
            "nav{background:rgba(245,245,247,.85);backdrop-filter:blur(20px);"
            "border-bottom:1px solid var(--border);position:sticky;top:0;z-index:100;"
            "display:flex;align-items:center;justify-content:space-between;"
            "padding:0 40px;height:52px;}"
            ".nav-brand{font-size:.95rem;font-weight:600}"
            "main{max-width:680px;margin:0 auto;padding:60px 20px 80px}"
            ".hero{text-align:center;margin-bottom:60px}"
            ".hero h1{font-size:2.8rem;font-weight:700;letter-spacing:-.5px;margin-bottom:12px}"
            ".hero p{font-size:1.1rem;color:var(--muted)}"
            ".card{background:var(--white);border-radius:var(--radius-lg);overflow:hidden;"
            "border:1px solid var(--border);margin-bottom:16px;}"
            ".section-header{padding:24px 28px 20px;border-bottom:1px solid var(--bg)}"
            ".section-label{font-size:.72rem;font-weight:600;color:var(--muted);"
            "letter-spacing:.6px;text-transform:uppercase;margin-bottom:6px}"
            ".section-title{font-size:1.05rem;font-weight:600}"
            ".section-body{padding:20px 28px 24px;display:flex;flex-direction:column;gap:12px}"
            "input,textarea{width:100%;padding:12px 14px;border:1.5px solid var(--border);"
            "border-radius:10px;font-size:.95rem;font-family:inherit;color:var(--text);"
            "background:var(--bg);outline:none;transition:border-color .15s;}"
            "input:focus,textarea:focus{border-color:var(--blue);background:var(--white)}"
            "textarea{resize:vertical;min-height:80px}"
            "button{padding:13px;width:100%;border:none;border-radius:10px;"
            "font-size:.92rem;font-weight:600;font-family:inherit;cursor:pointer;transition:opacity .15s;}"
            "button:hover{opacity:.84}.btn-blue{background:var(--blue);color:#fff}"
            ".btn-dark{background:var(--text);color:#fff}.btn-green{background:var(--green);color:#fff}"
            ".result{padding:14px 16px;background:var(--bg);border-radius:10px;"
            "border:1.5px solid var(--border);font-family:monospace;font-size:.82rem;"
            "line-height:1.65;white-space:pre-wrap;min-height:44px;display:none}"
            ".result.show{display:block}.result.error{color:#d70015}"
            ".result.loading{color:var(--muted)}"
            "</style></head><body>"
            "<nav><span class=\"nav-brand\">NachOS FS</span></nav>"
            "<main>"
            "<div class=\"hero\"><h1>Filesystem remoto</h1>"
            "<p>Acceso al sistema de archivos de NachOS.</p></div>"
            "<div class=\"card\"><div class=\"section-header\">"
            "<div class=\"section-label\">GET</div>"
            "<div class=\"section-title\">Obtener figura</div></div>"
            "<div class=\"section-body\">"
            "<input id=\"g-name\" placeholder=\"Nombre de la figura\""
            " onkeydown=\"if(event.key==='Enter')get()\"/>"
            "<button class=\"btn-blue\" onclick=\"get()\">Obtener</button>"
            "<div class=\"result\" id=\"g-res\"></div></div></div>"
            "<div class=\"card\"><div class=\"section-header\">"
            "<div class=\"section-label\">GET</div>"
            "<div class=\"section-title\">Listar todas las figuras</div></div>"
            "<div class=\"section-body\">"
            "<button class=\"btn-dark\" onclick=\"list()\">Ver lista</button>"
            "<div class=\"result\" id=\"l-res\"></div></div></div>"
            "<div class=\"card\"><div class=\"section-header\">"
            "<div class=\"section-label\">POST</div>"
            "<div class=\"section-title\">Guardar figura</div></div>"
            "<div class=\"section-body\">"
            "<input id=\"p-name\" placeholder=\"Nombre de la figura\"/>"
            "<textarea id=\"p-body\" placeholder=\"Contenido\"></textarea>"
            "<button class=\"btn-green\" onclick=\"post()\">Guardar</button>"
            "<div class=\"result\" id=\"p-res\"></div></div></div>"
            "</main>"
            "<script>"
            "const base=()=>'http://127.0.0.1:8080';"
            "function parse(text){"
            "  const m=text.match(/<result>(.*?)<\\/result>/s);"
            "  return m?m[1].trim():text;"
            "}"
            "function show(id,text,state){"
            "  const el=document.getElementById(id);"
            "  el.textContent=parse(text);"
            "  el.className='result show'+(state==='error'?' error':state==='loading'?' loading':'');}"
            "async function get(){"
            "  const n=document.getElementById('g-name').value.trim();"
            "  if(!n){show('g-res','Escribí el nombre.','error');return}"
            "  show('g-res','…','loading');"
            "  try{const r=await fetch(base()+'/figura/'+n);"
            "  show('g-res',await r.text(),r.ok?'':'error')}"
            "  catch(e){show('g-res',e.message,'error')}}"
            "async function list(){"
            "  show('l-res','…','loading');"
            "  try{const r=await fetch(base()+'/figuras');"
            "  show('l-res',await r.text(),r.ok?'':'error')}"
            "  catch(e){show('l-res',e.message,'error')}}"
            "async function post(){"
            "  const n=document.getElementById('p-name').value.trim();"
            "  const b=document.getElementById('p-body').value.trim();"
            "  if(!n||!b){show('p-res','Completá los campos.','error');return}"
            "  show('p-res','…','loading');"
            "  try{const r=await fetch(base()+'/figura/'+n,"
            "  {method:'POST',headers:{'Content-Type':'text/plain'},body:b});"
            "  show('p-res',await r.text(),r.ok?'':'error')}"
            "  catch(e){show('p-res',e.message,'error')}}"
            "</script></body></html>\n";

        std::string resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n" + html;

        client->Write(resp.c_str());
        client->Close();
        return;
    }

    std::string proto;

    if (method == "GET" && path.find("/figura/") == 0) {
        proto = "P/G/" + getFigureName(path);
    }
    else if (method == "GET" && path == "/figuras") {
        proto = "P/R/";
    }
    else if (method == "POST" && path.find("/figura/") == 0) {
        std::string name = getFigureName(path);
        std::string body = getBody(request);

        name = resolveUniqueName(name);

        /* Replace pipes with newlines to match the format used by the
         * original HTTP client (consistent storage format) */
        proto = "P/W/" + name + "|" + pipesToNewlines(body);

        log("[POST] Guardando como: " + name);
    }
    else {
        client->Write(buildResponse(400, "Ruta no valida").c_str());
        client->Close();
        return;
    }

    log("[PROTO] Enviado al server: " + proto);

    SSLSocket s('s');

    try {
        s.MakeConnection(SERVER_IP.c_str(), SERVER_PORT.c_str());
        s.InitClient();
        s.DoSSLConnect();
        s.Write(proto.c_str());

        char serverBuffer[BUFSIZE] = {0};
        int bytes = s.Read(serverBuffer, BUFSIZE - 1);
        serverBuffer[bytes] = '\0';

        std::string response(serverBuffer);
        log("[SERVER] Respuesta: " + response);

        if (response.find("ERROR") != std::string::npos) {
            client->Write(buildResponse(404, response).c_str());
        } else {
            std::string data = (response.size() > 4) ? response.substr(4) : response;

            if (proto.rfind("P/W/", 0) == 0) {
                size_t sep = proto.find("|");
                std::string savedAs = (sep != std::string::npos)
                    ? proto.substr(4, sep - 4)
                    : proto.substr(4);
                data = "Guardado como: " + savedAs;
            }

            client->Write(buildResponse(200, data).c_str());
        }

    } catch (...) {
        log("[CRITICAL] Fallo conexión con servidor");
        client->Write(buildResponse(500, "Server unreachable").c_str());
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

    std::thread clientLoop([&]() {
        while (true) {
            VSocket* client = server.AcceptConnection();
            std::thread(handleClient, client).detach();
        }
    });

    std::thread intLoop([&]() {
        while (true) {
            VSocket* c = intServer.AcceptConnection();
            std::thread(handleIntermediary, c).detach();
        }
    });

    clientLoop.detach();
    intLoop.detach();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}