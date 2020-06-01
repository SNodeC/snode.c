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

    int incManaged() {
        return ++ManagedCounter::managedCounter;
    }

    int decManaged() {
        return --ManagedCounter::managedCounter;
    }

    virtual void unmanaged() = 0;

    bool managed = false;
};

#endif // MANAGEABLE_H
