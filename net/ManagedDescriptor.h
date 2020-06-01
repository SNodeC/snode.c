#ifndef MANAGEABLE_H
#define MANAGEABLE_H

#include "Descriptor.h"
#include "ManagedCounter.h"


class ManagedDescriptor : virtual public ManagedCounter {
public:
    ManagedDescriptor()
        : managed(false) {
    }

    virtual ~ManagedDescriptor() = default;

    void incManaged() {
        ManagedCounter::managedCounter++;
    }

    void decManaged() {
        ManagedCounter::managedCounter--;
        if (ManagedCounter::managedCounter == 0) {
            unmanaged();
        }
    }

    virtual void unmanaged() = 0;

    bool managed = false;
};

#endif // MANAGEABLE_H
