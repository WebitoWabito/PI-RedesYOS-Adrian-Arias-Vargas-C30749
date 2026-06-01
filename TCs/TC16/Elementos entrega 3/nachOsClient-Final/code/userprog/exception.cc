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
#include "synch.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <netinet/in.h>


// I create this active process table to solve the problem of pointer truncation to 32 bits in MIPS.
// When I passed a pointer directly, it ended up being truncated, which caused a seg fault.
// To solve this, I created a global table that maps a small int (SpaceId) to a Thread* pointer. 
// This way, I can pass the SpaceId as an int without losing the pointer information, and I can look up the Thread* in the table when needed.

// Max number of processes, because literally its an array
#define MAX_PROCESSES 64

static Thread* processTable[MAX_PROCESSES];
static int     nextSpaceId = 1;   // I use this to start from another position, not just from 1

/**
 * This method registers a Thread* in the processTable and returns a unique SpaceId (index) for it.
 */
static int registerProcess(Thread *thread) {
    for (int i = nextSpaceId; i < MAX_PROCESSES; i++) {
        if (processTable[i] == NULL) {
            processTable[i] = thread;
            nextSpaceId = i + 1;
            if (nextSpaceId >= MAX_PROCESSES) nextSpaceId = 1; // This is to simulate a circular list
            return i;
        }
    }
    // This second loop is to check from the beginning of the table in case we have wrapped around
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (processTable[i] == NULL) {
            processTable[i] = thread;
            return i;
        }
    }
    return -1;  // No space available
}

/**
 * Looks up a Thread pointer in the process table using the given id.
 */
static Thread* lookupProcess(int spaceId) {
    if (spaceId <= 0 || spaceId >= MAX_PROCESSES) return NULL;
    return processTable[spaceId];
}

/**
 * Unregisters a process from the table, setting its entry to NULL.
 */
static void unregisterProcess(int spaceId) {
    if (spaceId > 0 && spaceId < MAX_PROCESSES)
        processTable[spaceId] = NULL;
}

/**
 * This method updates the program counter. It is used to advance the PC.
 */
void returnFromSystemCall() {
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg,     machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

/**
 * This method is to read a string from the user memory. It reads byte by byte until it finds a null terminator or reaches the maximum length.
 */
void readStrFromUser(int addr, char *buf, int maxLen) {
    int c;
    for (int i = 0; i < maxLen - 1; i++) {

        machine->ReadMem(addr + i, 1, &c);
        buf[i] = (char) c;

        if (c == '\0') {
            return;
        }
    }

    buf[maxLen - 1] = '\0';
}

/**
 * Function to run a thread created by Fork. It initializes the registers, restores the state, sets the program counter to the user function, and runs the machine.
 */
static void NachosForkThread(void *p) {

    AddrSpace *space = currentThread->space; // Obtein the address space of the current thread (the parent)
    space->InitRegisters(); // Initialize the registers of the address space
    space->RestoreState();  // Load page table
    machine->WriteRegister(RetAddrReg, 4);
    machine->WriteRegister(PCReg,     (long) p);
    machine->WriteRegister(NextPCReg, (long) p + 4);
    machine->Run();

    ASSERT(false);
}

/**
 * It's the entry point for the process created by exec. It initializes the registers, restores the state, and runs the machine. 
 * the Assert its used to ensure that never returns
 */
static void ExecAux(void *arg) {
    currentThread->space->InitRegisters(); // Initilize CPU registers
    currentThread->space->RestoreState(); // Load page table
    machine->Run(); // Run the machine, never returns
    ASSERT(false);
}

// System calls

/*
 *  System call interface: Halt()
 */

void NachOS_Halt() {
    DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
}

/*
 *  System call interface: void Exit( int )
 * The idea that I followed here its that I have to wake up parents, free resources,
 * delete table and finish the thread.
 */
void NachOS_Exit() {
    DEBUG('u', "Exit system call\n");

    Semaphore *joinSem = (Semaphore *) currentThread->joinSemaphore;
    if (joinSem != NULL) { // If some thread is waiting for this one to finish, wake it up
        currentThread->joinSemaphore = NULL;
        joinSem->V(); 
    }

    // Here just check if the process is registered in the table, if it is, unregister it. This is to avoid leaving dangling pointers in the table.
    if (currentThread->spaceId > 0) {
        unregisterProcess(currentThread->spaceId); // Unregister the process from the table
        currentThread->spaceId = 0;
    }


    // Free memory 
    delete currentThread->space;
    currentThread->space = NULL;
    currentThread->Finish();
}

/*
 *  System call interface: SpaceId Exec( char * )
 *  Returns a SpaceId (index in table, fits in 32 bits).
 */

void NachOS_Exec() {

    DEBUG('u', "Exec system call\n");
    int addr = machine->ReadRegister(4);
    char name[256];
    readStrFromUser(addr, name, 256); // Read the name of the executable from user memory

    OpenFile *executable = fileSystem->Open(name); // This opens the executable file from the file system

    if (executable == NULL) {
        printf("NachOS_Exec: no se pudo abrir \"%s\"\n", name);
        machine->WriteRegister(2, -1); // Returns an error to the user
        returnFromSystemCall();
        return;
    }

    // Create a new thread for the process, and an address space for it. 
    // The address space is initialized with the executable file. Then we delete the executable file because it's no longer needed.
    Thread *newThread = new Thread(name);
    newThread->space  = new AddrSpace(executable); // Create a new addres spece (code, stack, page table) for the new process
    delete executable; // I don't need this anymore

    // This semaphore is used for join.
    newThread->joinSemaphore = (void *) new Semaphore("joinSem", 0); // Creates a sem for join

    // Register the new process in the table and get its SpaceId
    int spaceId = registerProcess(newThread);
    newThread->spaceId = spaceId;

    // Return the spaceId to the user
    // This is where I had to create the process array, because it gave me some seg faults when I tried to pass the pointer directly, 
    // because it was truncated to 32 bits. 
    machine->WriteRegister(2, spaceId);
    newThread->Fork(ExecAux, NULL); // Starts the new thread, which will run the user program. The user program will start executing in the ExecAux function, 
                                    //which will initialize the registers and run the machine.
    returnFromSystemCall();
}

/*
 *  System call interface: int Join( SpaceId )
 *  This just waits for the child process to finish. It looks up the child process in the table, 
 *  and if it exists, it waits on its join semaphore.
 */

void NachOS_Join() {

    DEBUG('u', "Join system call\n");
    int spaceId = machine->ReadRegister(4);

    Thread *child = lookupProcess(spaceId); // Search for the child process in the table using the spaceId provided by the user

    //If doesn't exists, returns an error
    if (child == NULL || child->joinSemaphore == NULL) {
        machine->WriteRegister(2, -1);
        returnFromSystemCall();
        return;
    }

    // Save the join semaphore before waiting, because the child might call Exit and destroy it
    Semaphore *joinSem = (Semaphore *) child->joinSemaphore;

    joinSem->P();   // wait for the child to finish

    delete joinSem; 

    machine->WriteRegister(2, 0); 
    returnFromSystemCall();
}

/*
 *  System call interface: void Create( char * )
 */

void NachOS_Create() {

    int addr = machine->ReadRegister(4);
    char name[256];
    readStrFromUser(addr, name, 256); // copy the name of the file from user memory to kernel memory
    int fd = creat(name, 0644); // Create the file with read/write permissions for owner and read permissions for group and others. 
                                // Returns a file descriptor or -1 on error.

    machine->WriteRegister(2, (fd == -1) ? -1 : 0); // return 0 on success, -1 on error

    if (fd != -1) {
        ::close(fd); // This is important because we just want to create the file, not keep it open.
    }

    returnFromSystemCall();
}

/*
 *  System call interface: OpenFileId Open( char * )
 */

void NachOS_Open() {

    int addr = machine->ReadRegister(4);
    char name[256];
    readStrFromUser(addr, name, 256);

    int unixFd = open(name, O_RDWR | O_CREAT, 0644); // Open the file with read/write permissions, create it if it 
                                                     // doesn't exist. Returns a file descriptor or -1 on error.

    // If the file couldn't be opened, return an error. Otherwise, register the unix file descriptor in the file table 
    // of the current process and return the handle to the user.
    if (unixFd == -1) { 
        machine->WriteRegister(2, -1);
        returnFromSystemCall();
        return;
    }

    int handle = currentThread->space->fileTable->Open(unixFd);
    if (handle == -1) { 
        ::close(unixFd); 
    }

    machine->WriteRegister(2, handle); // Connect file with process
    returnFromSystemCall();
}

/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 *  Reads from keyboard and from files
 */

void NachOS_Read() {

    int addr = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    int file = machine->ReadRegister(6);

    char *buffer = new char[size + 1]; // Creates kernel buffer to read the data.
    int bytes = 0;

    // Reading from from keyboard
    if (file == ConsoleInput) {
        bytes = read(0, buffer, size);
    } else {
        // Verify if the file is open, if not, return an error and free the buffer.
        if (!currentThread->space->fileTable->isOpened(file)) {
            machine->WriteRegister(2, -1);
            delete[] buffer;
            returnFromSystemCall();
            return;
        }

        // Reading from a file
        bytes = read(currentThread->space->fileTable->getUnixHandle(file),
                     buffer, size);
    }

    // Copy the data from the kernel buffer to user memory, byte by byte.
    for (int i = 0; i < bytes; i++)
        machine->WriteMem(addr + i, 1, buffer[i]); 

    delete[] buffer;
    machine->WriteRegister(2, bytes); // Return the number of bytes read
    returnFromSystemCall();
}

/*
 *  System call interface: OpenFileId Write( char *, int, OpenFileId )
 */
void NachOS_Write() {

    int addr = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    int file = machine->ReadRegister(6);

    char *buffer = new char[size + 1];
    int c;

    // Copy the data from user memory to the kernel buffer, byte by byte.
    for (int i = 0; i < size; i++) {
        machine->ReadMem(addr + i, 1, &c);
        buffer[i] = (char) c; // Saves each byte read from user memory into the kernel buffer
    }
    buffer[size] = '\0'; // Null-terminate the buffer just in case


    // We need to manage some cases
    switch (file) {
        // Doesn't make sense to write to the console input, so we return an error
        case ConsoleInput:
            machine->WriteRegister(2, -1);
            break;
        // Standard console
        case ConsoleOutput:
            write(1, buffer, size);
            machine->WriteRegister(2, size);
            break;
        // Standard error
        case ConsoleError:
            printf("%d\n", machine->ReadRegister(4));
            machine->WriteRegister(2, size);
            break;
        // Other files
        default:
            if (!currentThread->space->fileTable->isOpened(file)) {
                machine->WriteRegister(2, -1);
            } else {
                int written = write(
                    currentThread->space->fileTable->getUnixHandle(file),
                    buffer, size);
                machine->WriteRegister(2, written);
            }
            break;
    }
    delete[] buffer;
    returnFromSystemCall();
}

/*
 *  System call interface: void Close( OpenFileId )
 *  Close an opened file.
 */
void NachOS_Close() {

    int handle = machine->ReadRegister(4);

    // Verify if it exists, if not, return an error. Otherwise, close the file and remove it from the file table.
    if (!currentThread->space->fileTable->isOpened(handle)) {
        machine->WriteRegister(2, -1);
        returnFromSystemCall();
        return;
    }

    // If the file is open, we close it and remove it from the file table
    ::close(currentThread->space->fileTable->getUnixHandle(handle));
    currentThread->space->fileTable->Close(handle);
    machine->WriteRegister(2, 0);
    returnFromSystemCall();
}

/*
 *  System call interface: void Fork( void (*func)() )
 *  Creates a new thread that runs the same program as the parent, but starts executing in the user function provided as an argument.
 */
void NachOS_Fork() {
    DEBUG('u', "Fork system call\n");
    long userFunc = (long) machine->ReadRegister(4); // I get the function

    Thread *newThread = new Thread("forked thread"); // Create a new thread for the child process
    newThread->space = new AddrSpace(currentThread->space); // Copy the address space of the parent process to the child process.
    newThread->joinSemaphore = NULL;
    newThread->spaceId = 0;  // I don't register the child process in the table because it's not needed, and it will never be joined.

    newThread->Fork(NachosForkThread, (void *) userFunc);
    returnFromSystemCall();
}

/*
 *  System call interface: void Yield()
 */
void NachOS_Yield() {
    returnFromSystemCall();
    currentThread->Yield();
}


/*
 *  System call interface: Sem_t SemCreate( int )
 */

void NachOS_SemCreate() {
    Semaphore *sem = new Semaphore("userSem", machine->ReadRegister(4));
    machine->WriteRegister(2, (int)(long) sem);
    returnFromSystemCall();
}

/*
 *  System call interface: int SemDestroy( Sem_t )
 */
void NachOS_SemDestroy() {
    Semaphore *sem = (Semaphore *)(long)(unsigned int) machine->ReadRegister(4);
    if (sem) delete sem;
    machine->WriteRegister(2, 0);
    returnFromSystemCall();
}


/*
 *  System call interface: int SemSignal( Sem_t )
 */
void NachOS_SemSignal() {
    Semaphore *sem = (Semaphore *)(long)(unsigned int) machine->ReadRegister(4);
    if (sem) sem->V();
    machine->WriteRegister(2, 0);
    returnFromSystemCall();
}

/*
 *  System call interface: int SemWait( Sem_t )
 */
void NachOS_SemWait() {
    Semaphore *sem = (Semaphore *)(long)(unsigned int) machine->ReadRegister(4);
    if (sem) sem->P();
    machine->WriteRegister(2, 0);
    returnFromSystemCall();
}

/*
 *  System call interface: Lock_t LockCreate( int )
 */
void NachOS_LockCreate(){ 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}

/*
 *  System call interface: int LockDestroy( Lock_t )
 */
void NachOS_LockDestroy() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}

/*
 *  System call interface: int LockAcquire( Lock_t )
 */
void NachOS_LockAcquire() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}

/*
 *  System call interface: int LockRelease( Lock_t )
 */
void NachOS_LockRelease() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}

/*
 *  System call interface: Cond_t LockCreate( int )
 */
void NachOS_CondCreate() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}


/*
 *  System call interface: int CondDestroy( Cond_t )
 */
void NachOS_CondDestroy() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}

/*
 *  System call interface: int CondSignal( Cond_t )
 */
void NachOS_CondSignal() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}

/*
 *  System call interface: int CondWait( Cond_t )
 */
void NachOS_CondWait() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}


/*
 *  System call interface: int CondBroadcast( Cond_t )
 */
void NachOS_CondBroadcast() { 
    machine->WriteRegister(2,0); returnFromSystemCall(); 
}


/*
 *  System call interface: Socket_t Socket( int, int )
 */
void NachOS_Socket() {
    // Decides the address family
    int af   = (machine->ReadRegister(4) == 0) ? AF_INET : AF_INET6;
    // Decides the socket type
    int sock = (machine->ReadRegister(5) == 0) ? SOCK_STREAM : SOCK_DGRAM;

    // Create the socket and return the handle to the user. If the socket couldn't be created, return an error.
    int fd   = socket(af, sock, 0);
    machine->WriteRegister(2, fd == -1 ? -1 :currentThread->space->fileTable->Open(fd));
    returnFromSystemCall();
}

/*
 *  System call interface: Socket_t Connect( char *, int )
 */
void NachOS_Connect() {

    int handle = machine->ReadRegister(4);
    char ip[64];

    readStrFromUser(machine->ReadRegister(5), ip, 64); // Read ip address from user memory
    int port = machine->ReadRegister(6);
    int unixFd = currentThread->space->fileTable->getUnixHandle(handle);

    struct sockaddr_in server;
    server.sin_family = AF_INET; //This implementation only works with IPv4
    
    server.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);
    machine->WriteRegister(2,
        connect(unixFd, (struct sockaddr*)&server, sizeof(server)));
    returnFromSystemCall();
}


/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() { 
    returnFromSystemCall(); 
}

/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() { 
    returnFromSystemCall(); 
}

/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() { 
    returnFromSystemCall(); 
}

/*
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() {
    shutdown(currentThread->space->fileTable->getUnixHandle(
        machine->ReadRegister(4)), SHUT_RDWR);
    machine->WriteRegister(2, 0);
    returnFromSystemCall();
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

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2) - SC_Base;

    switch (which) {
        case SyscallException:
            switch (type) {
                case SC_Halt:          NachOS_Halt();          break;
                case SC_Exit:          NachOS_Exit();          break;
                case SC_Exec:          NachOS_Exec();          break;
                case SC_Join:          NachOS_Join();          break;
                case SC_Create:        NachOS_Create();        break;
                case SC_Open:          NachOS_Open();          break;
                case SC_Read:          NachOS_Read();          break;
                case SC_Write:         NachOS_Write();         break;
                case SC_Close:         NachOS_Close();         break;
                case SC_Fork:          NachOS_Fork();          break;
                case SC_Yield:         NachOS_Yield();         break;
                case SC_SemCreate:     NachOS_SemCreate();     break;
                case SC_SemDestroy:    NachOS_SemDestroy();    break;
                case SC_SemSignal:     NachOS_SemSignal();     break;
                case SC_SemWait:       NachOS_SemWait();       break;
                case SC_LckCreate:     NachOS_LockCreate();    break;
                case SC_LckDestroy:    NachOS_LockDestroy();   break;
                case SC_LckAcquire:    NachOS_LockAcquire();   break;
                case SC_LckRelease:    NachOS_LockRelease();   break;
                case SC_CondCreate:    NachOS_CondCreate();    break;
                case SC_CondDestroy:   NachOS_CondDestroy();   break;
                case SC_CondSignal:    NachOS_CondSignal();    break;
                case SC_CondWait:      NachOS_CondWait();      break;
                case SC_CondBroadcast: NachOS_CondBroadcast(); break;
                case SC_Socket:        NachOS_Socket();        break;
                case SC_Connect:       NachOS_Connect();       break;
                case SC_Bind:          NachOS_Bind();          break;
                case SC_Listen:        NachOS_Listen();        break;
                case SC_Accept:        NachOS_Accept();        break;
                case SC_Shutdown:      NachOS_Shutdown();      break;
                default:
                    printf("Unexpected syscall %d\n", type);
                    ASSERT(false); break;
            }
            break;
        case PageFaultException:   break;
        case ReadOnlyException:
            printf("Read Only exception (%d)\n", which); ASSERT(false); break;
        case BusErrorException:
            printf("Bus error exception (%d)\n", which); ASSERT(false); break;
        case AddressErrorException:
            printf("Address error exception (%d)\n", which); ASSERT(false); break;
        case OverflowException:
            printf("Overflow exception (%d)\n", which); ASSERT(false); break;
        case IllegalInstrException:
            printf("Illegal instruction exception (%d)\n", which); ASSERT(false); break;
        default:
            printf("Unexpected exception %d\n", which); ASSERT(false); break;
    }
}