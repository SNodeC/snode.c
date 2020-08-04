#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "ManagedExceptions.h"

// IWYU pragma: no_include "Exception.h"

int ManagedExceptions::dispatch(const fd_set& fdSet, int count) {
    for (auto [fd, exception] : descriptors) {
        if (FD_ISSET(fd, &fdSet)) {
            count--;
            exception->exceptionEvent();
        }
    }

    return count;
}
