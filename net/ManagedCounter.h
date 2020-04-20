#ifndef MANAGEDCOUNTER_H
#define MANAGEDCOUNTER_H

class ManagedCounter
{
public:
    ManagedCounter() : managedCount(0) {}
    
protected:
    int managedCount;
};

#endif // MANAGEDCOUNTER_H
