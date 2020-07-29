#ifndef MANAGEDDESCRIPTOR_H
#define MANAGEDDESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedCounter.h"

class ManagedDescriptor : virtual public ManagedCounter {
public:
    ManagedDescriptor() = default;

    ManagedDescriptor(const ManagedDescriptor&) = delete;

    ManagedDescriptor& operator=(const ManagedDescriptor&) = delete;

    virtual ~ManagedDescriptor() = default;

    void incManaged() {
        ManagedCounter::managedCounter++;
        managed = true;
    }

    void decManaged() {
        ManagedCounter::managedCounter--;
        managed = false;
    }

    void checkDangling() {
        if (ManagedCounter::managedCounter == 0) {
            unmanaged();
        }
    }

    bool isManaged() const {
        return managed;
    }

protected:
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void unmanaged() = 0;

private:
    bool managed{false};
};

#endif // MANAGEDDESCRIPTOR_H
