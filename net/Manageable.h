#ifndef MANAGEABLE_H
#define MANAGEABLE_H

#include <iostream>

#include "Descriptor.h"
#include "ManagedCounter.h"

class ManagedDescriptor : virtual public Descriptor, virtual public ManagedCounter
{   
public:
    ManagedDescriptor(int fd) : managed(false) {
        this->setFd(fd);
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
    
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
    
public:
    bool managed = false;
};

#endif // MANAGEABLE_H
