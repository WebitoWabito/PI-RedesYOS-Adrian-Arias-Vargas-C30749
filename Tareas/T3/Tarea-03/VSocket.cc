/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-i
  *  Grupos: 2 y 3
  *
  *******   VSocket base class implementation
  *
  * (Fedora version)
  *
  **/

#include <sys/socket.h>
#include <arpa/inet.h>      // ntohs, htons
#include <stdexcept>            // runtime_error
#include <cstring>      // memset
#include <netdb.h>      // getaddrinfo, freeaddrinfo
#include <unistd.h>     // close
/*
#include <cstddef>
#include <cstdio>

//#include <sys/types.h>
*/
#include "VSocket.h"


/**
  *  Class creator (constructor)
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
  **/
void VSocket::Init( char t, bool IPv6 ){
   int domain;
   int typeSock;
   this->type = t;
   this->IPv6 = IPv6;

   domain = (IPv6) ? AF_INET6 : AF_INET;

   if ( t == 's' )
      typeSock = SOCK_STREAM;
   else
      typeSock = SOCK_DGRAM;

   this->sockId = socket(domain, typeSock, 0);

   if ( this->sockId == -1 ) {
      throw std::runtime_error("VSocket::Init socket()");
   }
}


/**
  * Class destructor
  *
 **/
VSocket::~VSocket() {

   this->Close();

}


/**
  * Close method
  *    use Unix close system call (once opened a socket is managed like a file in Unix)
  *
 **/
void VSocket::Close(){
   if (this->sockId >= 0) {
      int st = close(this->sockId);
      this->sockId = -1;

      if ( st == -1 ) {
         throw std::runtime_error("VSocket::Close()");
      }
   }
}


/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dot notation, example "10.84.166.62"
  * @param      int port: process address, example 80
  *
 **/
int VSocket::TryToConnect( const char * hostip, int port ) {
   int st;

   if (!this->IPv6) {
      struct sockaddr_in host4;
      memset(&host4, 0, sizeof(host4));
      host4.sin_family = AF_INET;
      st = inet_pton(AF_INET, hostip, &host4.sin_addr);

      if ( st <= 0 )
         throw std::runtime_error("VSocket::TryToConnect inet_pton IPv4");

      host4.sin_port = htons(port);
      st = connect(this->sockId, (struct sockaddr*)&host4, sizeof(host4));

      if ( st == -1 )
         throw std::runtime_error("VSocket::TryToConnect connect IPv4");

   } else {
      struct sockaddr_in6 host6;
      memset(&host6, 0, sizeof(host6));
      host6.sin6_family = AF_INET6;
      st = inet_pton(AF_INET6, hostip, &host6.sin6_addr);

      if ( st <= 0 )
         throw std::runtime_error("VSocket::TryToConnect inet_pton IPv6");

      host6.sin6_port = htons(port);
      st = connect(this->sockId, (struct sockaddr*)&host6, sizeof(host6));

      if ( st == -1 )
         throw std::runtime_error("VSocket::TryToConnect connect IPv6");

   }
   return st;
}


/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int VSocket::TryToConnect( const char *host, const char *service ) {
   struct addrinfo hints, *result, *rp;
   int st;
   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   st = getaddrinfo(host, service, &hints, &result);

   if ( st != 0 )
      throw std::runtime_error("VSocket::TryToConnect getaddrinfo");

   for ( rp = result; rp != NULL; rp = rp->ai_next ) {
      st = connect(this->sockId, rp->ai_addr, rp->ai_addrlen);
      if ( st == 0 )
         break;
   }

   freeaddrinfo(result);
   if ( rp == NULL )
      throw std::runtime_error("VSocket::TryToConnect connect failed");

   return st;
}


int VSocket::MakeConnection( const char *host, int port ) {
   return this->Connect( host, port );
}

int VSocket::MakeConnection( const char *host, const char *service ) {
   return this->Connect( host, service );
}


/**
  * Bind method
  *    use "bind" Unix system call (man 3 bind) (server mode)
  *
  * @param      int port: bind a unamed socket to a port defined in sockaddr structure
  *
  *  Links the calling process to a service at port
  *
 **/
int VSocket::Bind( int port ) {
   int st = -1;

   if (!this->IPv6) {
      struct sockaddr_in host4;
      memset(&host4, 0, sizeof(host4));
      host4.sin_family = AF_INET;
      host4.sin_port = htons(port);
      host4.sin_addr.s_addr = htonl(INADDR_ANY);
      st = bind(this->sockId, (struct sockaddr*)&host4, sizeof(host4));
   } else {
      struct sockaddr_in6 host6;
      memset(&host6, 0, sizeof(host6));
      host6.sin6_family = AF_INET6;
      host6.sin6_port = htons(port);
      host6.sin6_addr = in6addr_any;
      st = bind(this->sockId, (struct sockaddr*)&host6, sizeof(host6));
   }

   if ( st == -1) {
      throw std::runtime_error( "VSocket::Bind" );
   }

   return st;
}


/**
  *  sendTo method
  *
  *  @param const void * buffer: data to send
  *  @param size_t size data size to send
  *  @param void * addr address to send data
  *
  *  Send data to another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::sendTo( const void * buffer, size_t size, void * addr ) {
   int st = -1;

   if(!IPv6) {
      struct sockaddr* address = (struct sockaddr*) addr;
      socklen_t addrlen = sizeof(sockaddr_in);
      st = sendto(this->sockId, buffer, size, 0, address, addrlen);
   } else {
      struct sockaddr_in6* address = (struct sockaddr_in6*) addr;
      socklen_t addrlen = sizeof(sockaddr_in6);
      st = sendto(this->sockId, buffer, size, 0, (struct sockaddr*)address, addrlen);
   }

   if (st == -1) {
      throw std::runtime_error( "VSocket::sendTo" );
   }

   return st;
}


/**
  *  recvFrom method
  *
  *  @param const void * buffer: data to send
  *  @param size_t size data size to send
  *  @param void * addr address to receive from data
  *
  *  @return size_t bytes received
  *
 **/
size_t VSocket::recvFrom( void * buffer, size_t size, void * addr ) {
   ssize_t length = -1;
   struct sockaddr* address = (struct sockaddr*) addr;
   socklen_t addrlen = sizeof(struct sockaddr_in);

   if(!IPv6) {
      addrlen = sizeof(struct sockaddr_in);
      length = recvfrom(this->sockId, buffer, size, 0, address, &addrlen);
   } else {
      struct sockaddr_in6* address6 = (struct sockaddr_in6*) addr;
      addrlen = sizeof(struct sockaddr_in6);
      length = recvfrom(this->sockId, buffer, size, 0, (struct sockaddr*)address6, &addrlen);
   }

   if (length == -1) {
      throw std::runtime_error( "VSocket::recvFrom" );
   }

   return (size_t)length;
}
