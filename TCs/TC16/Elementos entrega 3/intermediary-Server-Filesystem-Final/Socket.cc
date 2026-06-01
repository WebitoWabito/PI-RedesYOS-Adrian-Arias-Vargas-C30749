#include "Socket.h"

#include <unistd.h>
#include <cstring>
#include <stdexcept>

Socket::Socket(char t, bool IPv6) {
    this->BuildSocket(t, IPv6);
}

Socket::~Socket() {}

int Socket::MakeConnection(const char *host, const char *service) {
    return this->EstablishConnection(host, service);
}

int Socket::MakeConnection(const char *hostip, int port) {
    return this->EstablishConnection(hostip, port);
}

size_t Socket::Read(void *buffer, size_t size) {
    int st = read(this->idSocket, buffer, size);
    if (st == -1) throw std::runtime_error("Socket::Read");
    return st;
}

size_t Socket::Write(const void *buffer, size_t size) {
    int st = write(this->idSocket, buffer, size);
    if (st == -1) throw std::runtime_error("Socket::Write");
    return st;
}

size_t Socket::Write(const char *text) {
    int st = write(this->idSocket, text, strlen(text));
    if (st == -1) throw std::runtime_error("Socket::Write text");
    return st;
}

VSocket* Socket::AcceptConnection() {
    int newSocket = this->WaitForConnection();

    Socket* peer = new Socket('s', this->IPv6);
    peer->idSocket = newSocket;

    return peer;
}

void Socket::setSocket(int s) {
    this->idSocket = s;
}