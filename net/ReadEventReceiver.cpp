#include "ReadEventReceiver.h"

#include "EventLoop.h"
#include "ReadEventDispatcher.h" // for ManagedExceptions

void ReadEventReceiver::enable() {
    EventLoop::instance().getReadEventDispatcher().enable(this);
}

void ReadEventReceiver::disable() {
    EventLoop::instance().getReadEventDispatcher().disable(this);
}
