#ifndef WRITER_H
#define WRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"

class WriteEvent : public ManagedDescriptor {
public:
    ~WriteEvent() override = default;

    virtual void writeEvent() = 0;

    void start() override;
    void stop() override;
};

#endif // WRITER_H
