#include "SocketExceptionManager.h"


int SocketExceptionManager::process(fd_set& fdSet, int count) {
    for (std::list<Exception*>::iterator it = descriptors.begin(); it != descriptors.end() && count > 0; ++it) {
        if (FD_ISSET(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet)) {
            count--;
            (*it)->exceptionEvent();
        }
    }
    
    return count;
}
