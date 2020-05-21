#ifndef MANAGEABLE_H
#define MANAGEABLE_H

#include "Descriptor.h"
#include "ManagedCounter.h"


class ManagedDescriptor : virtual public Descriptor, virtual public ManagedCounter
{   
public:
    ManagedDescriptor() : Descriptor(getFd()), managed(false) {}

    virtual ~ManagedDescriptor() = default;
    
    void incManaged() {
        ManagedCounter::managedCounter++;
    }
    
    void decManaged() {
        ManagedCounter::managedCounter--;
        
        if (managedCounter == 0) {
            delete this;
        }
    }
    
//    virtual void unused() = 0;
    
    bool managed = false;
};

#endif // MANAGEABLE_H
