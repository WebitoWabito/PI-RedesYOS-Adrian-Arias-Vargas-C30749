#include "Socket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <cstdio>

Socket::Socket( char t, bool IPv6 ) {
   this->Init( t, IPv6 );
}

Socket::Socket( int id ) {
   this->Init( id );
}

Socket::~Socket() {
}

int Socket::Connect( const char * hostip, int port ) {
   return this->TryToConnect( hostip, port );
}

int Socket::Connect( const char * host, const char * service ) {
   return this->TryToConnect( host, service );
}

size_t Socket::Read( void * buffer, size_t size ) {
   ssize_t st = read(this->sockId, buffer, size);
   if ( st == -1 ) {
      throw std::runtime_error("Socket::Read failed");
   }
   return static_cast<size_t>( st );
}

size_t Socket::Write( const void * buffer, size_t size ) {
   ssize_t st = write(this->sockId, buffer, size);
   if ( st == -1 ) {
      throw std::runtime_error("Socket::Write failed");
   }
   return static_cast<size_t>( st );
}

size_t Socket::Write( const char * text ) {
   size_t len = strlen(text);
   ssize_t st = write(this->sockId, text, len);
   if ( st == -1 ) {
      throw std::runtime_error("Socket::Write failed");
   }
   return static_cast<size_t>( st );
}

VSocket * Socket::AcceptConnection() {
   int id = this->WaitForConnection();
   return new Socket( id );
}
