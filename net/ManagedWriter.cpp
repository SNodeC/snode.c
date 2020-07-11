#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedWriter.h"


int ManagedWriter::dispatch(const fd_set& fdSet, int count) {
    for (std::pair<int, Writer*> writerPair : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(writerPair.second)->getFd(), &fdSet)) {
            count--;
            writerPair.second->writeEvent();
        }
    }

    return count;
}
