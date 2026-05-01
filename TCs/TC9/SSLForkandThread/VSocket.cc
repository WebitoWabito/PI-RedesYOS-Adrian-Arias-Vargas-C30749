#include "VSocket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

void VSocket::Init( char t, bool IPv6 ) {
   int domain = IPv6 ? AF_INET6 : AF_INET;
   int typeSock = (t == 's') ? SOCK_STREAM : SOCK_DGRAM;
   this->sockId = socket(domain, typeSock, 0);
   if ( this->sockId == -1 ) {
      throw std::runtime_error("VSocket::Init socket creation failed");
   }
   this->IPv6 = IPv6;
   this->type = t;
}

void VSocket::Init( int id ) {
   this->sockId = id;
   this->IPv6 = false;
   this->type = 's';
}

VSocket::~VSocket() {
   this->Close();
}

void VSocket::Close() {
   if ( this->sockId >= 0 ) {
      close(this->sockId);
      this->sockId = -1;
   }
}

int VSocket::TryToConnect( const char * hostip, int port ) {
   struct sockaddr_in server_addr;
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(port);
   if ( inet_pton(AF_INET, hostip, &server_addr.sin_addr) <= 0 ) {
      throw std::runtime_error("VSocket::TryToConnect inet_pton failed");
   }

   int st = connect(this->sockId, (struct sockaddr*)&server_addr, sizeof(server_addr));
   if ( st == -1 ) {
      throw std::runtime_error("VSocket::TryToConnect failed");
   }

   return st;
}

int VSocket::TryToConnect( const char * host, const char * service ) {
   struct addrinfo hints;
   struct addrinfo *result;
   int st;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   st = getaddrinfo(host, service, &hints, &result);
   if ( st != 0 ) {
      throw std::runtime_error("VSocket::TryToConnect getaddrinfo failed");
   }

   for ( struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next ) {
      st = connect(this->sockId, rp->ai_addr, rp->ai_addrlen);
      if ( st == 0 )
         break;
   }

   freeaddrinfo(result);

   if ( st == -1 ) {
      throw std::runtime_error("VSocket::TryToConnect connect failed");
   }

   return st;
}

int VSocket::MakeConnection( const char * host, int port ) {
   return this->Connect( host, port );
}

int VSocket::MakeConnection( const char * host, const char * service ) {
   return this->Connect( host, service );
}

int VSocket::Bind( int port ) {
   int enable = 1;
   if ( setsockopt(this->sockId, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1 ) {
      throw std::runtime_error("VSocket::Bind setsockopt failed");
   }

   struct sockaddr_in server_addr;
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port = htons(port);

   int st = bind(this->sockId, (struct sockaddr*)&server_addr, sizeof(server_addr));
   if ( st == -1 ) {
      throw std::runtime_error("VSocket::Bind failed");
   }

   return st;
}

int VSocket::MarkPassive( int backlog ) {
   int st = listen(this->sockId, backlog);
   if ( st == -1 ) {
      throw std::runtime_error("VSocket::MarkPassive failed");
   }
   return st;
}

int VSocket::WaitForConnection( void ) {
   struct sockaddr_in client_addr;
   socklen_t len = sizeof(client_addr);
   int new_sock = accept(this->sockId, (struct sockaddr*)&client_addr, &len);
   if ( new_sock == -1 ) {
      throw std::runtime_error("VSocket::WaitForConnection failed");
   }
   return new_sock;
}

int VSocket::Shutdown( int mode ) {
   int st = shutdown(this->sockId, mode);
   if ( st == -1 ) {
      throw std::runtime_error("VSocket::Shutdown failed");
   }
   return st;
}

size_t VSocket::sendTo( const void * buffer, size_t size, void * addr ) {
   (void)buffer;
   (void)size;
   (void)addr;
   throw std::runtime_error("VSocket::sendTo not implemented in this folder");
}

size_t VSocket::recvFrom( void * buffer, size_t size, void * addr ) {
   (void)buffer;
   (void)size;
   (void)addr;
   throw std::runtime_error("VSocket::recvFrom not implemented in this folder");
}
