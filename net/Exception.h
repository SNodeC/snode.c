#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "Manageable.h"

class Exception : public Manageable {
public:
    virtual ~Exception() = default;
    
    virtual void exceptionEvent() = 0;
};


#endif // EXCEPTION_H
