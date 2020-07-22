#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
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
