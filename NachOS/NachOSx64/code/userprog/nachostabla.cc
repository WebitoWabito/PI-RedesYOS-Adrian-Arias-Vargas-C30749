#include "nachostabla.h"
#include <stdio.h>
 
 
NachosOpenFilesTable::NachosOpenFilesTable()
{
    openFiles    = new int[ MAX_OPEN_FILES ];
    openFilesMap = new BitMap( MAX_OPEN_FILES );
    usage        = 0;

    //se marcan en uso aunque no haya un archivo real detrás de ellos en la tabla
    openFilesMap->Mark(0);// stdin
    openFilesMap->Mark(1);// stdout
    openFilesMap->Mark(2);// stderr
 
    openFiles[0] = 0;
    openFiles[1] = 1;
    openFiles[2] = 2;
}
 
 
NachosOpenFilesTable::~NachosOpenFilesTable()
{
    delete [] openFiles;
    delete openFilesMap;
}
 
 
int NachosOpenFilesTable::Open( int unixHandle )
{
    int nachosHandle = openFilesMap->Find(); // busca la primera posición libre
    if ( nachosHandle == -1 ) {
        return -1; // tabla llena
    }
    openFiles[ nachosHandle ] = unixHandle;
    return nachosHandle;
}
 
 
int NachosOpenFilesTable::Close( int nachosHandle )
{
    if ( !isOpened( nachosHandle ) ) {
        return -1;
    }
    openFilesMap->Clear( nachosHandle );
    openFiles[ nachosHandle ] = -1;
    return 0;
}
 
 
bool NachosOpenFilesTable::isOpened( int nachosHandle )
{
    if ( nachosHandle < 0 || nachosHandle >= MAX_OPEN_FILES ) {
        return false;
    }
    return openFilesMap->Test( nachosHandle );
}
 
 
int NachosOpenFilesTable::getUnixHandle( int nachosHandle )
{
    if ( !isOpened( nachosHandle ) ) {
        return -1;
    }
    return openFiles[ nachosHandle ];
}
 
 
void NachosOpenFilesTable::addThread()
{
    usage++;
}
 
 
void NachosOpenFilesTable::delThread()
{
    usage--;
}
 
 
void NachosOpenFilesTable::Print()
{
    printf("NachosOpenFilesTable: usage=%d\n", usage);
    for ( int i = 0; i < MAX_OPEN_FILES; i++ ) {
        if ( openFilesMap->Test(i) ) {
            printf("  NachOS handle %d -> Unix fd %d\n", i, openFiles[i]);
        }
    }
}