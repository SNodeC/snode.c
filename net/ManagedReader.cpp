#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedReader.h"


int ManagedReader::dispatch(const fd_set& fdSet, int count) {
    for (Reader* reader : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(reader)->fd(), &fdSet)) {
            count--;
            reader->readEvent();
        }
    }

    return count;
}
