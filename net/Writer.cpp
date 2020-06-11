#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"

#include "Multiplexer.h"


void Writer::stash() {
    Multiplexer::instance().getManagedWriter().stash(this);
}


void Writer::unstash() {
    Multiplexer::instance().getManagedWriter().unstash(this);
}
