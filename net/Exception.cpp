#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Exception.h"

#include "Multiplexer.h"


void Exception::stash() {
    Multiplexer::instance().getManagedExceptions().stash(this);
}


void Exception::unstash() {
    Multiplexer::instance().getManagedExceptions().unstash(this);
}
