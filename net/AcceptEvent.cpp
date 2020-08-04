#include "AcceptEvent.h"

#include "ManagedServer.h" // for ManagedExceptions
#include "Multiplexer.h"

void AcceptEvent::start() {
    Multiplexer::instance().getManagedServer().start(this);
}

void AcceptEvent::stop() {
    Multiplexer::instance().getManagedServer().stop(this);
}
