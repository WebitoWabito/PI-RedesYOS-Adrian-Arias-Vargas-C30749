#ifndef SSLSocket_h
#define SSLSocket_h

#include "Socket.h"

class SSLSocket : public Socket {
   public:
      SSLSocket( bool IPv6 = false );
      SSLSocket( const char *, const char *, bool = false );
      SSLSocket( int );
      ~SSLSocket();

      int Connect( const char *, int );
      int Connect( const char *, const char * );
      size_t Write( const char * );
      size_t Write( const void *, size_t );
      size_t Read( void *, size_t );
      void ShowCerts();
      const char * GetCipher();

      void Accept();
      void Copy( SSLSocket * original );
      VSocket * AcceptConnection() override;

   private:
      void InitSSL( bool = false );
      void InitContext( bool );
      void LoadCertificates( const char *, const char * );

      void * Context;
      void * BIO;
};

#endif
