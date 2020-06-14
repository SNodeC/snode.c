#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedExceptions.h"


int ManagedExceptions::dispatch(const fd_set& fdSet, int count) {
    for (Exception* exception : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(exception)->fd(), &fdSet)) {
            count--;
            exception->exceptionEvent();
        }
    }

    return count;
}
