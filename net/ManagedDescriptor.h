#ifndef MANAGEDDESCRIPTOR_H
#define MANAGEDDESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "ManagedCounter.h"


class ManagedDescriptor : virtual public ManagedCounter {
public:
    ManagedDescriptor() = default;

    ManagedDescriptor(const ManagedDescriptor&) = delete;

    ManagedDescriptor& operator=(const ManagedDescriptor&) = delete;

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

    //    bool managed = false;
};

#endif // MANAGEDDESCRIPTOR_H
