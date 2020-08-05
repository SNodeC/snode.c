#ifndef EXCEPTION_H
#define EXCEPTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventReceiver.h"

class OutOfBandEventReceiver : public EventReceiver {
public:
    ~OutOfBandEventReceiver() override = default;

    virtual void outOfBandEvent() = 0;

    void enable() override;
    void disable() override;
};

#endif // EXCEPTION_H
