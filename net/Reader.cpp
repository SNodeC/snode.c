#include "Reader.h"

#include "ManagedReader.h" // for ManagedExceptions
#include "Multiplexer.h"

void Reader::start() {
    Multiplexer::instance().getManagedReader().start(this);
}

void Reader::stop() {
    Multiplexer::instance().getManagedReader().stop(this);
}
