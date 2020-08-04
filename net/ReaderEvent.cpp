#include "ReaderEvent.h"

#include "ManagedReader.h" // for ManagedExceptions
#include "Multiplexer.h"

void ReadEvent::start() {
    Multiplexer::instance().getManagedReader().start(this);
}

void ReadEvent::stop() {
    Multiplexer::instance().getManagedReader().stop(this);
}
