#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"

#include "Multiplexer.h"


void Reader::stash() {
    Multiplexer::instance().getManagedReader().stash(this);
}


void Reader::unstash() {
    Multiplexer::instance().getManagedReader().unstash(this);
}
