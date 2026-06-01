// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

// addrspace.h
#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "nachosTable.h"

#define UserStackSize   4096

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);
    AddrSpace(AddrSpace *parentSpace);
    ~AddrSpace();

    void InitRegisters();
    void SaveState();
    void RestoreState();

    NachosOpenFilesTable *fileTable;

  private:
    TranslationEntry *pageTable;
    unsigned int numPages;
    bool isFork;        // true = constructor de copia, destructor solo libera stack
    int sharedPages;    // cuántas páginas son compartidas (no liberar en destructor)
};

#endif

