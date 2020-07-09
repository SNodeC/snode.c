#ifndef WRITER_H
#define WRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Writer : public ManagedDescriptor {
public:
    ~Writer() override = default;

    virtual void writeEvent() = 0;

protected:
    void start();
    void stop();
};

#endif // WRITER_H
