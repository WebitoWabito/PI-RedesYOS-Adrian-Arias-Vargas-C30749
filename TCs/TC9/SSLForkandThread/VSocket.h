#ifndef VSocket_h
#define VSocket_h

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <cstddef>

class VSocket {
   public:
      void Init( char, bool = false );
      void Init( int );
      ~VSocket();

      void Close();
      int TryToConnect( const char *, int );
      int TryToConnect( const char *, const char * );
      virtual int Connect( const char *, int ) = 0;
      virtual int Connect( const char *, const char * ) = 0;

      int MakeConnection( const char *, int );
      int MakeConnection( const char *, const char * );

      virtual size_t Read( void *, size_t ) = 0;
      virtual size_t Write( const void *, size_t ) = 0;
      virtual size_t Write( const char * ) = 0;

      int Bind( int );
      int MarkPassive( int );
      int WaitForConnection( void );
      virtual VSocket * AcceptConnection() = 0;
      int Shutdown( int );

      size_t sendTo( const void *, size_t, void * );
      size_t recvFrom( void *, size_t, void * );

   protected:
      int sockId;
      bool IPv6;
      int port;
      char type;
};

#endif // VSocket_h
