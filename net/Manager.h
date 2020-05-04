#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


template <typename T> class Manager
{
protected:
    Manager() : maxFd(0) {
        FD_ZERO(&fdSet);
    }

    
    virtual ~Manager() = default;

public:
    fd_set& getFdSet() {
        updateFdSet();
        
        return fdSet;
    }
    
    
    void manageSocket(T* socket) {
        if (!socket->managed) {
            addedDescriptors.push_back(socket);
            socket->managed = true;
        }
    }
    
    
    void unmanageSocket(T* socket) {
        if (socket->managed) {
            removedDescriptors.push_back(socket);
            socket->managed = false;
        }
    }
    
    
    int getMaxFd() {
        return maxFd;
    }
    

    void clear() {
        addedDescriptors.clear();
        removedDescriptors = descriptors;
        updateFdSet();
    }
    
    virtual int process(fd_set& fdSet, int count) = 0;

protected:
    std::list<T*> descriptors;

    
private:
    int updateMaxFd() {
        maxFd = 0;
        
        for (typename std::list<T*>::iterator it = descriptors.begin(); it != descriptors.end(); ++it) {
            if ((*it)->getFd() > maxFd) {
                maxFd = (*it)->getFd();
            }
        }
        
        return maxFd;
    }
    
    void updateFdSet() {
        if (!addedDescriptors.empty() || !removedDescriptors.empty()) {
            for (typename std::list<T*>::iterator it = addedDescriptors.begin(); it != addedDescriptors.end(); ++it) {
                if (std::find(descriptors.begin(), descriptors.end(), *it) == descriptors.end()) {
                    FD_SET((*it)->getFd(), &fdSet);
                    descriptors.push_back(*it);
                    (*it)->incManaged();
                }
            }
            addedDescriptors.clear();
        
            for (typename std::list<T*>::iterator it = removedDescriptors.begin(); it != removedDescriptors.end(); ++it) {
                if (std::find(descriptors.begin(), descriptors.end(), *it) != descriptors.end()) {
                    FD_CLR((*it)->getFd(), &fdSet);
                    descriptors.remove(*it);
                    (*it)->decManaged();
                }
            }
            removedDescriptors.clear();
        
            updateMaxFd();
        }
    }
    

    fd_set fdSet;
    int maxFd;
    
    std::list<T*> addedDescriptors;
    std::list<T*> removedDescriptors;
};

#endif // SOCKETMANAGER_H
