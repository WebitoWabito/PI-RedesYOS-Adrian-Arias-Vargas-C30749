#include "water.h"
#include "system.h"
#include <stdio.h>
 
Water::Water() {
    H = 0;
    O = 0;
    lock          = new Lock("lock");
    hydrogenQueue = new Condition("hydrogenQueue");
    oxygenQueue   = new Condition("oxygenQueue");
}
 
Water::~Water() {
    delete lock;
    delete hydrogenQueue;
    delete oxygenQueue;
}
 
void Water::hydrogen() {
    lock->Acquire();
 
    H++;
    printf("H llega, total: H=%d, O=%d\n", H, O);

    oxygenQueue->Signal(lock);
 
    hydrogenQueue->Wait(lock);
 
    printf("H formo parte de una molecula de agua, total: H=%d, O=%d\n", H, O);
    lock->Release();
}
 
void Water::oxygen() {
    lock->Acquire();
 
    O++;
    printf("O llega, total: H=%d, O=%d\n", H, O);
 
    while (H < 2) {
        oxygenQueue->Wait(lock);
    }
 
    H -= 2;
    O -= 1;
    //I just put this because by definition, every time a Oexits, it means water was created
    printf("Se formo agua y quedan H=%d, O=%d restantes \n", H, O);
 
    hydrogenQueue->Signal(lock);
    hydrogenQueue->Signal(lock);
 
    lock->Release();
}