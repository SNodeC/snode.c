#include "Writer.h"
#include "Multiplexer.h"

void Writer::start() {
    Multiplexer::instance().getManagedWriter().start(this);
}

void Writer::stop() {
    Multiplexer::instance().getManagedWriter().stop(this);
}

