#ifndef WATER_H
#define WATER_H
 
#include "synch.h"
 
class Water {
  private:
    int H;   // hidrógenos esperando
    int O;   // oxígenos esperando
 
    Lock*      lock;
    Condition* hydrogenQueue;
    Condition* oxygenQueue;
 
  public:
    Water();
    ~Water();
 
    void hydrogen();
    void oxygen();
};
 
#endif // WATER_H
 