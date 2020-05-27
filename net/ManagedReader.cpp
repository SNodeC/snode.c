#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedReader.h"


int ManagedReader::dispatch(fd_set& fdSet, int count) {
    for_each(descriptors.begin(), descriptors.end(), [&fdSet, &count](Reader* reader) -> void {
        if (FD_ISSET(dynamic_cast<Descriptor*>(reader)->getFd(), &fdSet)) {
            count--;
            reader->readEvent();
        }
    });

    return count;
}
