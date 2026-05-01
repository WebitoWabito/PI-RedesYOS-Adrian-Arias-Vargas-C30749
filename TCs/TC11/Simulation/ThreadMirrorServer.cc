#include <iostream>
#include <thread>
#include "FileSystem.h"
#include "SSLSocket.h"

#define PORT 1235
#define BUFSIZE 512

FileSystem fs("disk.bin", 100);

void task(VSocket* vclient) {
    SSLSocket* client = dynamic_cast<SSLSocket*>(vclient);

    char buffer[BUFSIZE] = {0};
    client->Read(buffer, BUFSIZE);

    std::string input(buffer);
    std::cout << "Raw request: " << input << std::endl;

    try {
        if (input.rfind("P/G/", 0) == 0) {
            std::string name = input.substr(4);
            Figure fig = fs.readFigure(name);

            if (fig.pieces.empty())
                client->Write("P/D/ERROR_404");
            else
                client->Write(("P/D/" + fig.serialize()).c_str());
        }
        else if (input.rfind("P/R/", 0) == 0) {
            std::string res;
            for (auto& f : fs.getDirectory().getFiles())
                res += f.name + "\n";

            client->Write(("P/D/" + res).c_str());
        }
        else if (input.rfind("P/W/", 0) == 0) {
            size_t sep = input.find("|");
            std::string name = input.substr(4, sep - 4);
            std::string data = input.substr(sep + 1);

            Figure fig = Figure::deserialize(name, data);
            fs.writeFigure(fig);

            client->Write("P/D/WRITE_SUCCESS");
        }
        else {
            client->Write("P/D/UNKNOWN_COMMAND");
        }
    } catch (...) {
        client->Write("P/D/ERROR_PROTOCOL");
    }

    client->Close();
    delete client;
}

int main() {
    SSLSocket server('s');

    server.Bind(PORT);
    server.MarkPassive(5);

    std::cout << "[SERVER] SSL listo en puerto " << PORT << std::endl;

    while (true) {
        SSLSocket* client =
            static_cast<SSLSocket*>(server.AcceptConnection());

        client->InitServer("cert.pem", "key.pem");
        client->DoSSLAccept();

        std::thread(task, client).detach();
    }
}