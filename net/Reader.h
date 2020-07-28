#ifndef READER_H
#define READER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Reader : public ManagedDescriptor {
public:
    ~Reader() override = default;

    virtual void readEvent() = 0;

    void start() override;
    void stop() override;
};


#endif // READER_H
