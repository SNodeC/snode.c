#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "ManagedDescriptor.h"


class Exception : public ManagedDescriptor {
public:
    virtual ~Exception() = default;
    
    virtual void exceptionEvent() = 0;
};


#endif // EXCEPTION_H
