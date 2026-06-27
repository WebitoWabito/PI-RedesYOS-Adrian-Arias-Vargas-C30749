#include "SSLSocket.h"
#include <stdexcept>

static void initSSLOnce() {
    static bool done = false;
    if (!done) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        done = true;
    }
}

SSLSocket::SSLSocket(char t, bool IPv6)
    : Socket(t, IPv6), ctx(nullptr), ssl(nullptr) {
    initSSLOnce();
}

SSLSocket::~SSLSocket() {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (ctx) {
        SSL_CTX_free(ctx);
    }
}

void SSLSocket::InitServer(const char* certFile, const char* keyFile) {
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) throw std::runtime_error("SSL_CTX_new failed");

    if (SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) <= 0)
        throw std::runtime_error("cert error");

    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM) <= 0)
        throw std::runtime_error("key error");

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, this->getSocket());
}

void SSLSocket::DoSSLAccept() {
    if (SSL_accept(ssl) <= 0)
        throw std::runtime_error("SSL_accept failed");
}

void SSLSocket::InitClient() {
    ctx = SSL_CTX_new(TLS_client_method());
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, this->getSocket());
}

void SSLSocket::DoSSLConnect() {
    if (SSL_connect(ssl) <= 0)
        throw std::runtime_error("SSL_connect failed");
}

size_t SSLSocket::Read(void* buffer, size_t size) {
    int r = SSL_read(ssl, buffer, size);
    if (r <= 0) throw std::runtime_error("SSL_read failed");
    return r;
}

size_t SSLSocket::Write(const void* buffer, size_t size) {
    int r = SSL_write(ssl, buffer, size);
    if (r <= 0) throw std::runtime_error("SSL_write failed");
    return r;
}

size_t SSLSocket::Write(const char* text) {
    return Write((void*)text, strlen(text));
}

VSocket* SSLSocket::AcceptConnection() {
    int fd = this->WaitForConnection();
    SSLSocket* client = new SSLSocket('s', false);
    client->idSocket = fd;
    return client;
}