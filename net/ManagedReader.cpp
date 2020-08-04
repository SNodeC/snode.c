#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map> // for map
#include <tuple>
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ManagedReader.h"

// IWYU pragma: no_include "Reader.h"

int ManagedReader::dispatch(const fd_set& fdSet, int count) {
    for (auto [fd, reader] : descriptors) {
        if (FD_ISSET(fd, &fdSet)) {
            count--;
            reader->readEvent();
        }
    }

    return count;
}
