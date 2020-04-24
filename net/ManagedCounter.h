#ifndef MANAGEDCOUNTER_H
#define MANAGEDCOUNTER_H

#include <iostream>

class ManagedCounter
{
public:
    ManagedCounter() : managedCounter(0) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
    
protected:
    int managedCounter;
};

#endif // MANAGEDCOUNTER_H
