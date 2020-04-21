#ifndef MANAGEABLE_H
#define MANAGEABLE_H

#include "Descriptor.h"
#include "ManagedCounter.h"

class Manageable : virtual public ManagedCounter
{   
public:
    Manageable() : managed(false) {}
    
    virtual ~Manageable() = default;
    
    void incManaged() {
        ManagedCounter::managedCounter++;
    }
    
    void decManaged() {
        ManagedCounter::managedCounter--;
        
        if (managedCounter == 0) {
            delete this;
        }
        
    }
    
public:
    bool managed = false;
};

#endif // MANAGEABLE_H
