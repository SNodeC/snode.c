#include "OutOfBandEvent.h"

#include "ManagedExceptions.h" // for ManagedExceptions
#include "Multiplexer.h"

void OutOfBandEvent::start() {
    Multiplexer::instance().getManagedExceptions().start(this);
}

void OutOfBandEvent::stop() {
    Multiplexer::instance().getManagedExceptions().stop(this);
}
