#include "WriteEventReceiver.h"

#include "EventLoop.h"
#include "WriteEventDispatcher.h" // for ManagedExceptions

void WriteEventReceiver::enable() {
    EventLoop::instance().getWriteEventDispatcher().enable(this);
}

void WriteEventReceiver::disable() {
    EventLoop::instance().getWriteEventDispatcher().disable(this);
}
