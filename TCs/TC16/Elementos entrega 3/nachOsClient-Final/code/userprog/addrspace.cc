// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include <algorithm>
#include <string.h>

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

// -----------------------------------------------------------------------
// loadSegment
// I had to make this function beacuse when I implemented real pagination, the simple read give me some problems 
// because the virtual address of the segment could be in different pages, so I need to read it page by page
// -----------------------------------------------------------------------
static void loadSegment(OpenFile *exec, int virtualAddr, int inFileAddr,
                        int size, TranslationEntry *pageTable)
{
    if (size == 0) return;

    int bytesLeft  = size;
    int fileOffset = inFileAddr;
    int virtCursor = virtualAddr;

    while (bytesLeft > 0) {
        int vPage        = virtCursor / PageSize;
        int offsetInPage = virtCursor % PageSize;
        int physBase     = pageTable[vPage].physicalPage * PageSize;
        int toRead       = std::min(PageSize - offsetInPage, bytesLeft);

        exec->ReadAt(&(machine->mainMemory[physBase + offsetInPage]),
                     toRead, fileOffset);

        virtCursor += toRead;
        fileOffset += toRead;
        bytesLeft  -= toRead;
    }
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    unsigned int size = noffH.code.size + noffH.initData.size
                      + noffH.uninitData.size + UserStackSize;
    numPages    = divRoundUp(size, PageSize);
    isFork      = false;
    sharedPages = 0;

    ASSERT(numPages <= (unsigned int)NumPhysPages);

    DEBUG('a', "Initializing address space, num pages %d\n", numPages);

    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        int frame = memoryMap->Find();
        ASSERT(frame != -1);
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = frame;
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
        bzero(&(machine->mainMemory[frame * PageSize]), PageSize);
    }

    // Load code and data segments into memory
    loadSegment(executable,
                noffH.code.virtualAddr, noffH.code.inFileAddr,
                noffH.code.size, pageTable);

    loadSegment(executable,
                noffH.initData.virtualAddr, noffH.initData.inFileAddr,
                noffH.initData.size, pageTable);

    fileTable = new NachosOpenFilesTable();
    fileTable->addThread();
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace — constructor Fork
//  I create this copy constructor for the fork system call. It creates a new address space
//----------------------------------------------------------------------
AddrSpace::AddrSpace(AddrSpace *parent)
{
    numPages    = parent->numPages;
    isFork      = true;
    pageTable   = new TranslationEntry[numPages];

    int stackPgs = divRoundUp(UserStackSize, PageSize);
    sharedPages  = (int)numPages - stackPgs;

    // Páginas compartidas: copiar la entrada (mismo frame físico)
    for (int i = 0; i < sharedPages; i++) {
        pageTable[i] = parent->pageTable[i];
    }

    // Páginas privadas: nuevo frame físico para el stack del hijo
    for (int i = sharedPages; i < (int)numPages; i++) {
        int frame = memoryMap->Find();
        ASSERT(frame != -1);
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = frame;
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
        bzero(&(machine->mainMemory[frame * PageSize]), PageSize);
    }

    // Compartir tabla de archivos con el padre
    fileTable = parent->fileTable;
    fileTable->addThread();
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------
AddrSpace::~AddrSpace()
{
    if (isFork) {
        // Solo liberar las páginas privadas del stack
        for (int i = sharedPages; i < (int)numPages; i++) {
            memoryMap->Clear(pageTable[i].physicalPage);
        }
    } else {
        // Liberar todas las páginas (proceso independiente)
        for (unsigned int i = 0; i < numPages; i++) {
            memoryMap->Clear(pageTable[i].physicalPage);
        }
    }
    delete [] pageTable;

    fileTable->delThread();
    if (fileTable->isLastThread()) {
        delete fileTable;
    }
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters()
{
    for (int i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);
    machine->WriteRegister(PCReg, 0);
    machine->WriteRegister(NextPCReg, 4);
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------
void AddrSpace::SaveState()   {}


//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------
void AddrSpace::RestoreState()
{
    machine->pageTable     = pageTable;
    machine->pageTableSize = numPages;
}

