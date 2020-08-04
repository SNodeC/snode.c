#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ManagedReader.h"

// IWYU pragma: no_include "Reader.h"

int ManagedReader::dispatch(const fd_set& fdSet, int count) {
    for (std::pair<int, Reader*> readerPair : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(readerPair.second)->getFd(), &fdSet)) {
            count--;
            readerPair.second->readEvent();
        }
    }

    return count;
}
