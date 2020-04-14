#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#include <algorithm>

#include <sys/select.h>
#include <list>

#include "Descriptor.h"

template <typename T> class SocketManager
{
public:
    SocketManager() : maxFd(0) {
        FD_ZERO(&fdSet);
    }

    virtual ~SocketManager() = default;

    fd_set& getFdSet() {
        for (typename std::list<T*>::iterator it = addedDescriptors.begin(); it != addedDescriptors.end(); ++it) {
            if (std::find(descriptors.begin(), descriptors.end(), *it) == descriptors.end()) {
                FD_SET(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet);
                descriptors.push_back(*it);
                dynamic_cast<Descriptor*>(*it)->incManagedCount();
            }
        }
        addedDescriptors.clear();

        for (typename std::list<T*>::iterator it = removedDescriptors.begin(); it != removedDescriptors.end(); ++it) {
            if (std::find(descriptors.begin(), descriptors.end(), *it) != descriptors.end()) {
                FD_CLR(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet);
                descriptors.remove(*it);
                dynamic_cast<Descriptor*>(*it)->decManagedCount();;
            }
        }
        removedDescriptors.clear();

        updateMaxFd();

        return fdSet;
    }
    
    
    void manageSocket(T* socket) {
        if (socket != 0 && std::find(addedDescriptors.begin(), addedDescriptors.end(), socket) == addedDescriptors.end()) {
            addedDescriptors.push_back(socket);
        }
    }
    
    
    void unmanageSocket(T* socket) {
        if (socket != 0 && std::find(removedDescriptors.begin(), removedDescriptors.end(), socket) == removedDescriptors.end()) {
            removedDescriptors.push_back(socket);
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
