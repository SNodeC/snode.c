#include "ReaderEvent.h"

#include "ManagedReader.h" // for ManagedExceptions
#include "Multiplexer.h"

void ReadEvent::enable() {
    Multiplexer::instance().getManagedReader().start(this);
}

void ReadEvent::disable() {
    Multiplexer::instance().getManagedReader().stop(this);
}
