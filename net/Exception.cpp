#include "Exception.h"

#include "ManagedExceptions.h" // for ManagedExceptions
#include "Multiplexer.h"

void Exception::start() {
    Multiplexer::instance().getManagedExceptions().start(this);
}

void Exception::stop() {
    Multiplexer::instance().getManagedExceptions().stop(this);
}
