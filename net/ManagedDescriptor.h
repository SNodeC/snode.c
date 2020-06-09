#ifndef MANAGEABLE_H
#define MANAGEABLE_H

#include "Descriptor.h"
#include "ManagedCounter.h"

#include <iostream>


class ManagedDescriptor : virtual public ManagedCounter {
public:
    ManagedDescriptor() {
    }

    virtual ~ManagedDescriptor() = default;

    void incManaged() {
        ManagedCounter::managedCounter++;
    }

    void decManaged() {
        ManagedCounter::managedCounter--;
    }
    
    void checkForEOF() {
//        std::cout << "ManagedCounter: " << ManagedCounter::managedCounter << std::endl;
        if (ManagedCounter::managedCounter == 0) {
            unmanaged();
        }
    }

    virtual void unmanaged() = 0;

//    bool managed = false;
};

#endif // MANAGEABLE_H
