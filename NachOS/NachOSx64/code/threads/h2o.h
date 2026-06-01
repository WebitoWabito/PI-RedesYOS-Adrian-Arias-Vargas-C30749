#include "synch.h"

class H2O {

public:
    H2O();
    ~H2O();
    void hydrogen();
    void oxygen();

private:
    Lock *lock;
    Condition *oxygenCond;
    Condition *hydrogenCond;
    int hydrogenCount;
    int oxygenCount;
};
