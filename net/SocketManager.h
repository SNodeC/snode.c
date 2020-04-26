#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#include <sys/select.h>

#include <algorithm>
#include <list>

#include "ManagedDescriptor.h"


template <typename T> class SocketManager
{
protected:
    SocketManager() : maxFd(0) {
        FD_ZERO(&fdSet);
    }

    virtual ~SocketManager() = default;

public:
    fd_set& getFdSet() {
        for (typename std::list<T*>::iterator it = addedDescriptors.begin(); it != addedDescriptors.end(); ++it) {
            if (std::find(descriptors.begin(), descriptors.end(), *it) == descriptors.end()) {
                FD_SET(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet);
                descriptors.push_back(*it);
                dynamic_cast<ManagedDescriptor*>(*it)->incManaged();
            }
        }
        addedDescriptors.clear();

        for (typename std::list<T*>::iterator it = removedDescriptors.begin(); it != removedDescriptors.end(); ++it) {
            if (std::find(descriptors.begin(), descriptors.end(), *it) != descriptors.end()) {
                FD_CLR(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet);
                descriptors.remove(*it);
                dynamic_cast<ManagedDescriptor*>(*it)->decManaged();
            }
        }
        removedDescriptors.clear();

        updateMaxFd();

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
    

protected:
    int updateMaxFd() {
        maxFd = 0;
        
        for (typename std::list<T*>::iterator it = descriptors.begin(); it != descriptors.end(); ++it) {
            if (dynamic_cast<Descriptor*>(*it)->getFd() > maxFd) {
                maxFd = dynamic_cast<Descriptor*>(*it)->getFd();
            }
        }
        
        return maxFd;
    }

    fd_set fdSet;
    int maxFd;
    
    std::list<T*> descriptors;
    std::list<T*> addedDescriptors;
    std::list<T*> removedDescriptors;
};

#endif // SOCKETMANAGER_H
