#ifndef EXCEPTION_H
#define EXCEPTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


#include "ManagedDescriptor.h"


class Exception : public ManagedDescriptor {
public:
    ~Exception() override = default;

    virtual void exceptionEvent() = 0;

protected:
    void start();
    void stop();
};


#endif // EXCEPTION_H
