// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//
// Copyright (c) -2025 Universidad de Costa Rica


#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "nachostabla.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

NachosOpenFilesTable * openFilesTable;

//Actualiza los contadores de programa de MIPS para que la simulación siga de manera correcta
// después de un syscall.
//se llama al final de cada syscall
void returnFromSystemCall() {
    machine->WriteRegister( PrevPCReg, machine->ReadRegister( PCReg ) );
    machine->WriteRegister( PCReg, machine->ReadRegister( NextPCReg ) );
    machine->WriteRegister( NextPCReg, machine->ReadRegister( NextPCReg ) + 4 );
}

/*
 *  System call interface: Halt()
 */
void NachOS_Halt() {          // System call 0
    DEBUG('a', "Halt system call.\n");
    interrupt->Halt();
}


/*
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() {          // System call 1
    DEBUG('u', "Exit system call\n");
 
    int status = machine->ReadRegister( 4 );
    DEBUG('u', "Exit status = %d\n", status);
 
    // se reducen los hilos en la tabla de archivos abiertos
    openFilesTable->delThread();
    currentThread->Finish();
}


/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {		// System call 2
}


/*
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() {		// System call 3
}


/*
 *  System call interface: void Create( char * )
 */
void NachOS_Create() {		// System call 4
}


/*
 *  System call interface: OpenFileId Open( char * )
 */
void NachOS_Open() {          // System call 5
    int addr = machine->ReadRegister( 4 );
    char nombre[256];
    int  car;
    int  i = 0;
 
    DEBUG('u', "Open system call\n");
 
    //se lee el nombre caracter por caracter desde memoria de usuario
    do {
        machine->ReadMem( addr + i, 1, &car );
        nombre[i] = (char) car;
        i++;
    } while ( car != 0 && i < 255 );
    nombre[i] = '\0';
 
    DEBUG('u', "Open: archivo = \"%s\"\n", nombre);
 
    int unixFd = open( nombre, O_RDWR );
    if ( unixFd == -1 ) {
        unixFd = open( nombre, O_RDONLY );
    }
 
    int nachosHandle = -1;
    if ( unixFd != -1 ) {
        // se registra en la tabla
        nachosHandle = openFilesTable->Open( unixFd );
        if ( nachosHandle == -1 ) {
            // si está llena la tabla se cierra 
            close( unixFd );
        }
    }
 
    machine->WriteRegister( 2, nachosHandle );
 
    returnFromSystemCall();
}


/*
 *  System call interface: OpenFileId Write( char *, int, OpenFileId )
 */
void NachOS_Write() {         // System call 7
    int addr = machine->ReadRegister( 4 );
    int size = machine->ReadRegister( 5 );
    int descriptor = machine->ReadRegister( 6 );
    int car;
 
    DEBUG('u', "Write system call\n");
 
    //se lee el buffer desde la memoria del user
    char * buffer = new char[ size + 1 ];
    for ( int i = 0; i < size; i++ ) {
        machine->ReadMem( addr + i, 1, &car );
        buffer[i] = (char) car;
    }
    buffer[size] = '\0';
 
    switch ( descriptor ) {
        case ConsoleInput:
            machine->WriteRegister( 2, -1 );
            break;
 
        case ConsoleOutput:
            printf( "%s", buffer );
            machine->WriteRegister( 2, size );
            break;
 
        case ConsoleError:
            printf( "%d\n", machine->ReadRegister( 4 ) );
            machine->WriteRegister( 2, size );
            break;
 
        default:
            if ( !openFilesTable->isOpened( descriptor ) ) {
                machine->WriteRegister( 2, -1 );
            } else {
                int unixFd  = openFilesTable->getUnixHandle( descriptor );
                int written = write( unixFd, buffer, size );
                machine->WriteRegister( 2, written );
            }
            break;
    }
 
    delete [] buffer;
    returnFromSystemCall();
}


/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 */
void NachOS_Read() {        // System call 6
    int addr       = machine->ReadRegister( 4 );
    int size       = machine->ReadRegister( 5 );
    int descriptor = machine->ReadRegister( 6 );
    int bytesRead  = 0;

    DEBUG('u', "Read system call\n");

    char * buffer = new char[ size + 1 ];

    switch ( descriptor ) {
        case ConsoleInput: //leer desde stdin
            for ( int i = 0; i < size; i++ ) {
                buffer[i] = getchar();
                bytesRead++;
            }
            buffer[bytesRead] = '\0';
            //se copia de vuelta a la memoria de user
            for ( int i = 0; i < bytesRead; i++ ) {
                machine->WriteMem( addr + i, 1, (int) buffer[i] );
            }
            machine->WriteRegister( 2, bytesRead );
            break;

        case ConsoleOutput://no se puede leer de stdout
            machine->WriteRegister( 2, -1 );
            break;

        default: //archivo abierto
            if ( !openFilesTable->isOpened( descriptor ) ) {
                machine->WriteRegister( 2, -1 );
            } else {
                int unixFd = openFilesTable->getUnixHandle( descriptor );
                bytesRead  = read( unixFd, buffer, size );
                // se escribe en memoria de user lo leído
                for ( int i = 0; i < bytesRead; i++ ) {
                    machine->WriteMem( addr + i, 1, (int) buffer[i] );
                }
                machine->WriteRegister( 2, bytesRead );
            }
            break;
    }

    delete [] buffer;
    returnFromSystemCall();
}


/*
 *  System call interface: void Close( OpenFileId )
 */
void NachOS_Close() {		// System call 8
}


/*
 *  System call interface: void Fork( void (*func)() )
 */
void NachOS_Fork() {		// System call 9
}


/*
 *  System call interface: void Yield()
 */
void NachOS_Yield() {		// System call 10
}


/*
 *  System call interface: Sem_t SemCreate( int )
 */
void NachOS_SemCreate() {		// System call 11
}


/*
 *  System call interface: int SemDestroy( Sem_t )
 */
void NachOS_SemDestroy() {		// System call 12
}


/*
 *  System call interface: int SemSignal( Sem_t )
 */
void NachOS_SemSignal() {		// System call 13
}


/*
 *  System call interface: int SemWait( Sem_t )
 */
void NachOS_SemWait() {		// System call 14
}


/*
 *  System call interface: Lock_t LockCreate( int )
 */
void NachOS_LockCreate() {		// System call 15
}


/*
 *  System call interface: int LockDestroy( Lock_t )
 */
void NachOS_LockDestroy() {		// System call 16
}


/*
 *  System call interface: int LockAcquire( Lock_t )
 */
void NachOS_LockAcquire() {		// System call 17
}


/*
 *  System call interface: int LockRelease( Lock_t )
 */
void NachOS_LockRelease() {		// System call 18
}


/*
 *  System call interface: Cond_t LockCreate( int )
 */
void NachOS_CondCreate() {		// System call 19
}


/*
 *  System call interface: int CondDestroy( Cond_t )
 */
void NachOS_CondDestroy() {		// System call 20
}


/*
 *  System call interface: int CondSignal( Cond_t )
 */
void NachOS_CondSignal() {		// System call 21
}


/*
 *  System call interface: int CondWait( Cond_t )
 */
void NachOS_CondWait() {		// System call 22
}


/*
 *  System call interface: int CondBroadcast( Cond_t )
 */
void NachOS_CondBroadcast() {		// System call 23
}


/*
 *  System call interface: Socket_t Socket( int, int )
 */
void NachOS_Socket() {      // System call 30
    DEBUG('u', "Socket system call\n");

    // socket TCP (IPv4, stream)
    int unixFd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( unixFd == -1 ) {
        machine->WriteRegister( 2, -1 );
        returnFromSystemCall();
        return;
    }

    int nachosHandle = openFilesTable->Open( unixFd );
    if ( nachosHandle == -1 ) {
        close( unixFd );
    }

    machine->WriteRegister( 2, nachosHandle );
    returnFromSystemCall();
}


/*
 *  System call interface: Socket_t Connect( char *, int )
 */
void NachOS_Connect() {     // System call 31
    int  addrPtr = machine->ReadRegister( 4 );// string con la IP
    int  port    = machine->ReadRegister( 5 );// número de puerto
    int  car;
    int  i = 0;
    char ipStr[256];

    DEBUG('u', "Connect system call\n");

    //se lee la IP desde memoria de user
    do {
        machine->ReadMem( addrPtr + i, 1, &car );
        ipStr[i] = (char) car;
        i++;
    } while ( car != 0 && i < 255 );
    ipStr[i] = '\0';

    int nachosHandle = machine->ReadRegister( 6 );

    if ( !openFilesTable->isOpened( nachosHandle ) ) {
        machine->WriteRegister( 2, -1 );
        returnFromSystemCall();
        return;
    }

    int unixFd = openFilesTable->getUnixHandle( nachosHandle );

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons( port );
    inet_pton( AF_INET, ipStr, &serverAddr.sin_addr );

    int result = connect( unixFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr) );

    machine->WriteRegister( 2, result );  // 0 si éxito, -1 si falla
    returnFromSystemCall();
}


/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() {		// System call 32
}


/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() {		// System call 33
}


/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() {		// System call 34
}


/*
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() {	// System call 25
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2) - SC_Base;

    switch ( which ) {

       case SyscallException:
          switch ( type ) {
             case SC_Halt:		// System call # 0
                NachOS_Halt();
                break;
             case SC_Exit:		// System call # 1
                NachOS_Exit();
                break;
             case SC_Exec:		// System call # 2
                NachOS_Exec();
                break;
             case SC_Join:		// System call # 3
                NachOS_Join();
                break;

             case SC_Create:		// System call # 4
                NachOS_Create();
                break;
             case SC_Open:		// System call # 5
                NachOS_Open();
                break;
             case SC_Read:		// System call # 6
                NachOS_Read();
                break;
             case SC_Write:		// System call # 7
                NachOS_Write();
                break;
             case SC_Close:		// System call # 8
                NachOS_Close();
                break;

             case SC_Fork:		// System call # 9
                NachOS_Fork();
                break;
             case SC_Yield:		// System call # 10
                NachOS_Yield();
                break;

             case SC_SemCreate:         // System call # 11
                NachOS_SemCreate();
                break;
             case SC_SemDestroy:        // System call # 12
                NachOS_SemDestroy();
                break;
             case SC_SemSignal:         // System call # 13
                NachOS_SemSignal();
                break;
             case SC_SemWait:           // System call # 14
                NachOS_SemWait();
                break;

             case SC_LckCreate:         // System call # 15
                NachOS_LockCreate();
                break;
             case SC_LckDestroy:        // System call # 16
                NachOS_LockDestroy();
                break;
             case SC_LckAcquire:         // System call # 17
                NachOS_LockAcquire();
                break;
             case SC_LckRelease:           // System call # 18
                NachOS_LockRelease();
                break;

             case SC_CondCreate:         // System call # 19
                NachOS_CondCreate();
                break;
             case SC_CondDestroy:        // System call # 20
                NachOS_CondDestroy();
                break;
             case SC_CondSignal:         // System call # 21
                NachOS_CondSignal();
                break;
             case SC_CondWait:           // System call # 22
                NachOS_CondWait();
                break;
             case SC_CondBroadcast:           // System call # 23
                NachOS_CondBroadcast();
                break;

             case SC_Socket:	// System call # 30
		NachOS_Socket();
               break;
             case SC_Connect:	// System call # 31
		NachOS_Connect();
               break;
             case SC_Bind:	// System call # 32
		NachOS_Bind();
               break;
             case SC_Listen:	// System call # 33
		NachOS_Listen();
               break;
             case SC_Accept:	// System call # 32
		NachOS_Accept();
               break;
             case SC_Shutdown:	// System call # 33
		NachOS_Shutdown();
               break;

             default:
                printf("Syscall no implementado: %d\n", type );
                printf("Unexpected syscall exception %d\n", type );
                ASSERT( false );
                break;
          }
          break;

       case PageFaultException: {
          break;
       }

       case ReadOnlyException:
          printf( "Read Only exception (%d)\n", which );
          ASSERT( false );
          break;

       case BusErrorException:
          printf( "Bus error exception (%d)\n", which );
          ASSERT( false );
          break;

       case AddressErrorException:
          printf( "Address error exception (%d)\n", which );
          ASSERT( false );
          break;

       case OverflowException:
          printf( "Overflow exception (%d)\n", which );
          ASSERT( false );
          break;

       case IllegalInstrException:
          printf( "Ilegal instruction exception (%d)\n", which );
          NachOS_Exit();
          break;

       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT( false );
          break;
    }

}
