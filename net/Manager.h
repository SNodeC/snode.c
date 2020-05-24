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


    virtual ~Manager() {
        descriptors.reverse();
        removedDescriptors = descriptors;
        updateFdSet();
    }


public:
    fd_set& getFdSet() {
        updateFdSet();

        return fdSet;
    }


    void add(T* socket) {
        if (!socket->managed) {
            addedDescriptors.push_back(socket);
            socket->managed = true;
        }
    }


    void remove(T* socket) {
        if (socket->managed) {
            removedDescriptors.push_back(socket);
            socket->managed = false;
        }
    }


    int getMaxFd() {
        return maxFd;
    }


    virtual int dispatch(fd_set& fdSet, int count) = 0;

protected:
    std::list<T*> descriptors;


private:
    int updateMaxFd() {
        maxFd = 0;

        for (typename std::list<T*>::iterator it = descriptors.begin(); it != descriptors.end(); ++it) {
            if (dynamic_cast<Descriptor*>(*it)->getFd() > maxFd) {
                maxFd = dynamic_cast<Descriptor*>(*it)->getFd();
            }
        }

        return maxFd;
    }

    void updateFdSet() {
        if (!addedDescriptors.empty() || !removedDescriptors.empty()) {
            for (typename std::list<T*>::iterator it = addedDescriptors.begin(); it != addedDescriptors.end(); ++it) {
                if (std::find(descriptors.begin(), descriptors.end(), *it) == descriptors.end()) {
                    FD_SET(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet);
                    descriptors.push_back(*it);
                    (*it)->incManaged();
                }
            }
            addedDescriptors.clear();

            for (typename std::list<T*>::iterator it = removedDescriptors.begin(); it != removedDescriptors.end(); ++it) {
                if (std::find(descriptors.begin(), descriptors.end(), *it) != descriptors.end()) {
                    FD_CLR(dynamic_cast<Descriptor*>(*it)->getFd(), &fdSet);
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
