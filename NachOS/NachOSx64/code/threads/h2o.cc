#include "h2o.h"
#include "system.h"

H2O::H2O() {
    lock = new Lock("h2o lock");
    oxygenCond = new Condition("oxygen condition");
    hydrogenCond = new Condition("hydrogen condition");
    hydrogenCount = 0;
    oxygenCount = 0;
}

H2O::~H2O() {
    delete lock;
    delete oxygenCond;
    delete hydrogenCond;
}

void H2O::hydrogen() {
    lock->Acquire();
    
    hydrogenCount++;
    
    // If we have 2 hydrogens and 1 oxygen, form water
    if (hydrogenCount >= 2 && oxygenCount >= 1) {
        hydrogenCount -= 2;
        oxygenCount -= 1;
        printf("H2O molecule formed!\n");
        
        // Signal other hydrogen and oxygen
        hydrogenCond->Signal(lock);
        hydrogenCond->Signal(lock);
        oxygenCond->Signal(lock);
    } else {
        // Wait for oxygen and another hydrogen
        hydrogenCond->Wait(lock);
    }
    
    lock->Release();
}

void H2O::oxygen() {
    lock->Acquire();
    
    oxygenCount++;
    
    if (hydrogenCount >= 2 && oxygenCount >= 1) {
        hydrogenCount -= 2;
        oxygenCount -= 1;
        printf("H2O molecule formed!\n");
        
        hydrogenCond->Signal(lock);
        hydrogenCond->Signal(lock);
        oxygenCond->Signal(lock);
    } else {
        oxygenCond->Wait(lock);
    }
    
    lock->Release();
}
