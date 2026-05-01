#include "VSocket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <cstdio>

void VSocket::BuildSocket(char t, bool IPv6) {
    this->type = t;
    this->IPv6 = IPv6;

    int domain = IPv6 ? AF_INET6 : AF_INET;
    int sockType = (t == 's') ? SOCK_STREAM : SOCK_DGRAM;

    this->idSocket = socket(domain, sockType, 0);

    if (this->idSocket == -1) {
        throw std::runtime_error("VSocket::BuildSocket failed");
    }

    if (domain == AF_INET6) {
        int no = 0;
        setsockopt(this->idSocket, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
    }
}

VSocket::~VSocket() {
    Close();
}

void VSocket::Close() {
    if (this->idSocket != -1) {
        close(this->idSocket);
        this->idSocket = -1;
    }
}

int VSocket::EstablishConnection(const char *host, const char *service) {
    struct addrinfo hints{}, *res, *p;

    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = (this->type == 's') ? SOCK_STREAM : SOCK_DGRAM;

    if (getaddrinfo(host, service, &hints, &res) != 0) {
        throw std::runtime_error("getaddrinfo failed");
    }

    int st = -1;

    for (p = res; p != NULL; p = p->ai_next) {
        st = connect(this->idSocket, p->ai_addr, p->ai_addrlen);
        if (st == 0) break;
    }

    freeaddrinfo(res);

    if (st == -1) {
        throw std::runtime_error("connect failed");
    }

    return st;
}

int VSocket::EstablishConnection(const char *hostip, int port) {
    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", port);
    return EstablishConnection(hostip, portStr);
}

int VSocket::Bind(int port) {
    struct addrinfo hints{}, *res, *p;
    char portStr[16];

    snprintf(portStr, sizeof(portStr), "%d", port);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, portStr, &hints, &res) != 0) {
        throw std::runtime_error("getaddrinfo failed in Bind");
    }

    int opt = 1;
    int st = -1;

    for (p = res; p != NULL; p = p->ai_next) {
        setsockopt(this->idSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        st = bind(this->idSocket, p->ai_addr, p->ai_addrlen);
        if (st == 0) break;
    }

    freeaddrinfo(res);

    if (st == -1) {
        perror("bind");
        throw std::runtime_error("Bind failed");
    }

    return st;
}

int VSocket::MarkPassive(int backlog) {
    if (listen(this->idSocket, backlog) == -1) {
        throw std::runtime_error("listen failed");
    }
    return 0;
}

int VSocket::WaitForConnection() {
    struct sockaddr_storage client;
    socklen_t len = sizeof(client);

    int st = accept(this->idSocket, (struct sockaddr*)&client, &len);

    if (st == -1) {
        throw std::runtime_error("accept failed");
    }

    return st;
}

int VSocket::Shutdown(int mode) {
    int st = shutdown(this->idSocket, mode);

    if (st == -1) {
        throw std::runtime_error("shutdown failed");
    }

    return st;
}

size_t VSocket::sendTo(const void *buffer, size_t size, void *addr) {
    return sendto(this->idSocket, buffer, size, 0,
                  (struct sockaddr*)addr,
                  sizeof(struct sockaddr_storage));
}

size_t VSocket::recvFrom(void *buffer, size_t size, void *addr) {
    socklen_t len = sizeof(struct sockaddr_storage);

    return recvfrom(this->idSocket, buffer, size, 0,
                    (struct sockaddr*)addr, &len);
}