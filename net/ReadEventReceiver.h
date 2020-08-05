#ifndef READER_H
#define READER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventReceiver.h"

class ReadEventReceiver : public EventReceiver {
public:
    ~ReadEventReceiver() override = default;

    virtual void readEvent() = 0;

    void enable() override;
    void disable() override;
};

#endif // READER_H
