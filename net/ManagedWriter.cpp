#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedWriter.h"


int ManagedWriter::dispatch(const fd_set& fdSet, int count) {
    for (Writer* writer : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(writer)->fd(), &fdSet)) {
            count--;
            writer->writeEvent();
        }
    }

    return count;
}
