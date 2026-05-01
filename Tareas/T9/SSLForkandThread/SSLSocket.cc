#include "SSLSocket.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdexcept>
#include <cstring>

SSLSocket::SSLSocket( bool IPv6 ) : Socket( 's', IPv6 ) {
   this->Context = nullptr;
   this->BIO = nullptr;
   this->InitSSL();
}

SSLSocket::SSLSocket( const char * certFileName, const char * keyFileName, bool IPv6 ) : Socket( 's', IPv6 ) {
   this->Context = nullptr;
   this->BIO = nullptr;
   this->InitSSL( true );
   this->LoadCertificates( certFileName, keyFileName );
}

SSLSocket::SSLSocket( int id ) : Socket( id ) {
   this->Context = nullptr;
   this->BIO = nullptr;
}

SSLSocket::~SSLSocket() {
   if ( this->BIO != nullptr ) {
      SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
      SSL_shutdown( ssl );
      SSL_free( ssl );
      this->BIO = nullptr;
   }

   if ( this->Context != nullptr ) {
      SSL_CTX_free( reinterpret_cast<SSL_CTX *>( this->Context ) );
      this->Context = nullptr;
   }
}

void SSLSocket::InitSSL( bool serverContext ) {
   if ( this->Context == nullptr ) {
      this->InitContext( serverContext );
   }

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( this->Context ) );
   if ( ssl == nullptr ) {
      throw std::runtime_error("SSLSocket::InitSSL SSL_new failed");
   }

   this->BIO = ssl;
}

void SSLSocket::InitContext( bool serverContext ) {
   const SSL_METHOD * method = nullptr;
   SSL_CTX * context = nullptr;

   if ( serverContext ) {
      OpenSSL_add_all_algorithms();
      SSL_load_error_strings();
      method = TLS_server_method();
   } else {
      OpenSSL_add_all_algorithms();
      SSL_load_error_strings();
      method = TLS_client_method();
   }

   if ( method == nullptr ) {
      throw std::runtime_error("SSLSocket::InitContext method creation failed");
   }

   context = SSL_CTX_new( method );
   if ( context == nullptr ) {
      throw std::runtime_error("SSLSocket::InitContext SSL_CTX_new failed");
   }

   this->Context = reinterpret_cast<void *>( context );
}

void SSLSocket::LoadCertificates( const char * certFileName, const char * keyFileName ) {
   SSL_CTX * ctx = reinterpret_cast<SSL_CTX *>( this->Context );
   if ( ctx == nullptr ) {
      throw std::runtime_error("SSLSocket::LoadCertificates no SSL context");
   }

   if ( SSL_CTX_use_certificate_file( ctx, certFileName, SSL_FILETYPE_PEM ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::LoadCertificates certificate load failed");
   }

   if ( SSL_CTX_use_PrivateKey_file( ctx, keyFileName, SSL_FILETYPE_PEM ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::LoadCertificates private key load failed");
   }

   if ( !SSL_CTX_check_private_key( ctx ) ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::LoadCertificates private key does not match certificate");
   }
}

int SSLSocket::Connect( const char * hostName, int port ) {
   int st = this->TryToConnect( hostName, port );

   if ( this->BIO == nullptr ) {
      this->InitSSL();
   }

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   if ( ssl == nullptr ) {
      throw std::runtime_error("SSLSocket::Connect no SSL object");
   }

   SSL_set_fd( ssl, this->sockId );
   st = SSL_connect( ssl );
   if ( st != 1 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::Connect SSL_connect failed");
   }

   return st;
}

int SSLSocket::Connect( const char * host, const char * service ) {
   int st = this->TryToConnect( host, service );

   if ( this->BIO == nullptr ) {
      this->InitSSL();
   }

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   if ( ssl == nullptr ) {
      throw std::runtime_error("SSLSocket::Connect no SSL object");
   }

   SSL_set_fd( ssl, this->sockId );
   st = SSL_connect( ssl );
   if ( st != 1 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::Connect SSL_connect failed");
   }

   return st;
}

size_t SSLSocket::Read( void * buffer, size_t size ) {
   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   if ( ssl == nullptr ) {
      throw std::runtime_error("SSLSocket::Read no SSL object");
   }

   int st = SSL_read( ssl, buffer, static_cast<int>( size ) );
   if ( st <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::Read SSL_read failed");
   }

   return static_cast<size_t>( st );
}

size_t SSLSocket::Write( const char * string ) {
   return this->Write( static_cast<const void *>( string ), strlen( string ) );
}

size_t SSLSocket::Write( const void * buffer, size_t size ) {
   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   if ( ssl == nullptr ) {
      throw std::runtime_error("SSLSocket::Write no SSL object");
   }

   int st = SSL_write( ssl, buffer, static_cast<int>( size ) );
   if ( st <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::Write SSL_write failed");
   }

   return static_cast<size_t>( st );
}

void SSLSocket::ShowCerts() {
   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   if ( ssl == nullptr ) {
      printf("No certificates, no SSL object.\n");
      return;
   }

   X509 * cert = SSL_get_peer_certificate( ssl );
   if ( cert != nullptr ) {
      char * line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
      printf( "Subject: %s\n", line );
      free( line );
      line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
      printf( "Issuer: %s\n", line );
      free( line );
      X509_free( cert );
   } else {
      printf( "No certificates.\n" );
   }
}

const char * SSLSocket::GetCipher() {
   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   if ( ssl == nullptr ) {
      return "";
   }
   return SSL_get_cipher( ssl );
}

void SSLSocket::Accept() {
   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   if ( ssl == nullptr ) {
      throw std::runtime_error("SSLSocket::Accept no SSL object");
   }

   int st = SSL_accept( ssl );
   if ( st != 1 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error("SSLSocket::Accept SSL_accept failed");
   }
}

void SSLSocket::Copy( SSLSocket * original ) {
   if ( original == nullptr ) {
      throw std::runtime_error("SSLSocket::Copy original is null");
   }

   SSL_CTX * ctx = reinterpret_cast<SSL_CTX *>( original->Context );
   if ( ctx == nullptr ) {
      throw std::runtime_error("SSLSocket::Copy original has no SSL context");
   }

   SSL * ssl = SSL_new( ctx );
   if ( ssl == nullptr ) {
      throw std::runtime_error("SSLSocket::Copy SSL_new failed");
   }

   SSL_set_fd( ssl, this->sockId );
   this->BIO = ssl;
}

VSocket * SSLSocket::AcceptConnection() {
   int id = this->WaitForConnection();
   return new SSLSocket( id );
}
