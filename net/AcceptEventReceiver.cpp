#include "AcceptEventReceiver.h"

#include "AcceptEventDispatcher.h" // for ManagedExceptions
#include "EventLoop.h"

void AcceptEventReceiver::enable() {
    EventLoop::instance().getAcceptEventDispatcher().enable(this);
}

void AcceptEventReceiver::disable() {
    EventLoop::instance().getAcceptEventDispatcher().disable(this);
}
