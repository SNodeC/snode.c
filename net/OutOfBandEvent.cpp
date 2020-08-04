#include "OutOfBandEvent.h"

#include "ManagedExceptions.h" // for ManagedExceptions
#include "Multiplexer.h"

void OutOfBandEvent::enable() {
    Multiplexer::instance().getManagedExceptions().start(this);
}

void OutOfBandEvent::disable() {
    Multiplexer::instance().getManagedExceptions().stop(this);
}
