#include "WriteEvent.h"

#include "ManagedWriter.h" // for ManagedExceptions
#include "Multiplexer.h"

void WriteEvent::start() {
    Multiplexer::instance().getManagedWriter().start(this);
}

void WriteEvent::stop() {
    Multiplexer::instance().getManagedWriter().stop(this);
}
