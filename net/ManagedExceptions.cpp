#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedExceptions.h"


int ManagedExceptions::dispatch(const fd_set& fdSet, int count) {
    for (std::pair<int, Exception*> exceptionPair : descriptors) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(exceptionPair.second)->getFd(), &fdSet)) {
            count--;
            exceptionPair.second->exceptionEvent();
        }
    }

    return count;
}
