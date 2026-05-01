#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#include "Socket.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

class SSLSocket : public Socket {
public:
    SSLSocket(char t, bool IPv6 = false);
    ~SSLSocket();

    void InitClient();
    void DoSSLConnect();

    void InitServer(const char* certFile, const char* keyFile);
    void DoSSLAccept();

    size_t Read(void* buffer, size_t size) override;
    size_t Write(const void* buffer, size_t size) override;
    size_t Write(const char* text) override;

    VSocket* AcceptConnection() override;

private:
    SSL_CTX* ctx;
    SSL* ssl;
};

#endif