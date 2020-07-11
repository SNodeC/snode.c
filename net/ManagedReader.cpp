#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedReader.h"


int ManagedReader::dispatch(const fd_set& fdSet, int count) {
    for (std::pair<int, Reader*> readerPair : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(readerPair.second)->getFd(), &fdSet)) {
            count--;
            readerPair.second->readEvent();
        }
    }

    return count;
}
