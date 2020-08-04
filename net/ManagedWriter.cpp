#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ManagedWriter.h"

// IWYU pragma: no_include "Writer.h"

int ManagedWriter::dispatch(const fd_set& fdSet, int count) {
    for (auto [fd, writer] : descriptors) {
        if (FD_ISSET(fd, &fdSet)) {
            count--;
            writer->writeEvent();
        }
    }

    return count;
}
