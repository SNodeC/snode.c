#ifndef READER_H
#define READER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Reader : public ManagedDescriptor {
public:
    ~Reader() override = default;

    virtual void readEvent() = 0;

protected:
    void start();
    void stop();
};


#endif // READER_H
