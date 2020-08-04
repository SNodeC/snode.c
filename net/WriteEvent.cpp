#include "WriteEvent.h"

#include "ManagedWriter.h" // for ManagedExceptions
#include "Multiplexer.h"

void WriteEvent::enable() {
    Multiplexer::instance().getManagedWriter().start(this);
}

void WriteEvent::disable() {
    Multiplexer::instance().getManagedWriter().stop(this);
}
