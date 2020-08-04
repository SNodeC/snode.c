#include "AcceptEvent.h"

#include "ManagedServer.h" // for ManagedExceptions
#include "Multiplexer.h"

void AcceptEvent::enable() {
    Multiplexer::instance().getManagedServer().start(this);
}

void AcceptEvent::disable() {
    Multiplexer::instance().getManagedServer().stop(this);
}
