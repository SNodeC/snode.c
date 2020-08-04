#ifndef READER_H
#define READER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"

class ReadEvent : public ManagedDescriptor {
public:
    ~ReadEvent() override = default;

    virtual void readEvent() = 0;

    void enable() override;
    void disable() override;
};

#endif // READER_H
