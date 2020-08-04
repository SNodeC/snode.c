#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ManagedWriter.h"
#include "Writer.h"

int ManagedWriter::dispatch(const fd_set& fdSet, int count) {
    for (std::pair<int, Writer*> writerPair : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(writerPair.second)->getFd(), &fdSet)) {
            count--;
            writerPair.second->writeEvent();
        }
    }

    return count;
}
