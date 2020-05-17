#include "ExceptionManager.h"


int ExceptionManager::dispatch(fd_set& fdSet, int count) {
    for (std::list<Exception*>::iterator it = descriptors.begin(); it != descriptors.end() && count > 0; ++it) {
        if (FD_ISSET((*it)->getFd(), &fdSet)) {
            count--;
            (*it)->exceptionEvent();
        }
    }
    
    return count;
}
