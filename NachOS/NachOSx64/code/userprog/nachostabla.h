// nachostabla.h
// Se maneja la tabla de archivos abiertos de un proceso
// de usuario en NachOS.
//
// Relaciona los "handles" de NachOS con los descriptores de Unix.
 
#ifndef NACHOSTABLA_H
#define NACHOSTABLA_H
 
#include "bitmap.h"
 
//Tamaño máximo de la tabla
#define MAX_OPEN_FILES 20
 
class NachosOpenFilesTable {
  public:
    NachosOpenFilesTable();
    ~NachosOpenFilesTable();
 
    int  Open( int unixHandle );// Registra un archivo abierto, retorna handle de NachOS
    int  Close( int nachosHandle );//Desregistra un archivo abierto
    bool isOpened( int nachosHandle );// Verifica si el handle está en uso
    int  getUnixHandle( int nachosHandle );// Retorna el descriptor Unix correspondiente
 
    void addThread();
    void delThread();
 
    void Print();
 
  private:
    int    * openFiles;      
    BitMap * openFilesMap;   
    int      usage;         
};
 
#endif