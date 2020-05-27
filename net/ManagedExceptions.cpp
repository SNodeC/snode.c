#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedExceptions.h"


int ManagedExceptions::dispatch(fd_set& fdSet, int count) {
    for_each(descriptors.begin(), descriptors.end(), [&fdSet, &count](Exception* exception) -> void {
        if (FD_ISSET(dynamic_cast<Descriptor*>(exception)->getFd(), &fdSet)) {
            count--;
            exception->exceptionEvent();
        }
    });

    return count;
}
