#include "OutOfBandEventReceiver.h"

#include "EventLoop.h"
#include "OutOfBandEventDispatcher.h" // for ManagedExceptions

void OutOfBandEventReceiver::enable() {
    EventLoop::instance().getOutOfBandEventDispatcher().enable(this);
}

void OutOfBandEventReceiver::disable() {
    EventLoop::instance().getOutOfBandEventDispatcher().disable(this);
}
