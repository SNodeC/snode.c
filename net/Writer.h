#ifndef WRITER_H
#define WRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Writer : public ManagedDescriptor {
public:
    ~Writer() override = default;

    virtual void writeEvent() = 0;

protected:
    std::string writePuffer;
};


#endif // WRITER_H
