/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  ****** SSLSocket example, server code with fork()
  *
  * (Fedora version)
  *
 **/

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "SSLSocket.h"

#define PORT	4321

void Service( SSLSocket * client ) {
   char buf[ 1024 ] = { 0 };
   int bytes;
   const char* ServerResponse="\n<Body>\n\
\t<Server>os.ecci.ucr.ac.cr</Server>\n\
\t<dir>ci0123</dir>\n\
\t<Name>Proyecto Integrador Redes y sistemas Operativos</Name>\n\
\t<NickName>PIRO</NickName>\n\
\t<Description>Consolidar e integrar los conocimientos de redes y sistemas operativos</Description>\n\
\t<Author>profesores PIRO</Author>\n\
</Body>\n";
    const char *validMessage = "\n<Body>\n\
\t<UserName>piro</UserName>\n\
\t<Password>ci0123</Password>\n\
</Body>\n";

   client->Accept();
   client->ShowCerts();

   bytes = client->Read( buf, sizeof( buf ) );
   buf[ bytes ] = '\0';
   printf( "Client msg: \"%s\"\n", buf );
   fflush( stdout );

   if ( ! strcmp( validMessage, buf ) ) {
      client->Write( ServerResponse, strlen( ServerResponse ) );
   } else {
      client->Write( "Invalid Message", strlen( "Invalid Message" ) );
   }

   client->Close();
   delete client;
}

void SignalHandler( int sig ) {
   while ( waitpid( -1, nullptr, WNOHANG ) > 0 );
}

int main( int cuantos, char ** argumentos ) {
   SSLSocket * server, * client;
   int port = PORT;
   bool ipv6 = false;
   pid_t pid;
   
   if ( cuantos > 1 ) {
      port = atoi( argumentos[ 1 ] );
   }
   if ( cuantos > 2 ) {
      ipv6 = (strcmp( argumentos[ 2 ], "ipv6" ) == 0);
   }

   signal( SIGCHLD, SignalHandler );

   server = new SSLSocket( (const char *) "ci0123.pem", (const char *) "key0123.pem", ipv6 );
   server->Bind( port );
   server->MarkPassive( 10 );

   printf( "SSL Server listening on port %d (IPv%d)...\n", port, ipv6 ? 6 : 4 );
   fflush( stdout );

   for( ; ; ) {
      client = (SSLSocket * ) server->AcceptConnection();
      client->Copy( server );
      
      pid = fork();
      if ( pid == 0 ) {
         Service( client );
         exit( 0 );
      } else if ( pid > 0 ) {
         client->Close();
         delete client;
      } else {
         perror( "fork" );
      }
   }

   return 0;
}
