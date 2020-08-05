#ifndef SERVER_H
#define SERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventReceiver.h"

class AcceptEventReceiver : public EventReceiver {
public:
    ~AcceptEventReceiver() override = default;

    virtual void acceptEvent() = 0;

    void enable() override;
    void disable() override;
};

#endif // SERVER_H
