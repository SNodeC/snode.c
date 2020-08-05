#ifndef WRITER_H
#define WRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventReceiver.h"

class WriteEventReceiver : public EventReceiver {
public:
    ~WriteEventReceiver() override = default;

    virtual void writeEvent() = 0;

    void enable() override;
    void disable() override;
};

#endif // WRITER_H
