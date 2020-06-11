#ifndef MANAGEDCOUNTER_H
#define MANAGEDCOUNTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class ManagedCounter {
public:
    ManagedCounter()
        : managedCounter(0) {
    }

protected:
    int managedCounter;
};

#endif // MANAGEDCOUNTER_H
