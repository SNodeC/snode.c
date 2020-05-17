#include "ReadManager.h"


int ReadManager::dispatch(fd_set& fdSet, int count) {
    for (std::list<Reader*>::iterator it = descriptors.begin(); it != descriptors.end() && count > 0; ++it) {
        if (FD_ISSET((*it)->getFd(), &fdSet)) {
            count--;
            (*it)->readEvent();
        }
    }
    
    return count;
}
