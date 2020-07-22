#include "Writer.h"

#include "ManagedWriter.h" // for ManagedExceptions
#include "Multiplexer.h"


void Writer::start() {
    Multiplexer::instance().getManagedWriter().start(this);
}

void Writer::stop() {
    Multiplexer::instance().getManagedWriter().stop(this);
}
