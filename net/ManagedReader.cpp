#include "ManagedReader.h"


int ManagedReader::dispatch(fd_set& fdSet, int count) {
    for (std::list<Reader*>::iterator it = descriptors.begin(); it != descriptors.end() && count > 0; ++it) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet)) {
            count--;
            (*it)->readEvent();
        }
    }

    return count;
}
