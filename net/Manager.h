#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


template <typename T> class Manager {
protected:
    Manager()
        : maxFd(0) {
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

        for_each(descriptors.begin(), descriptors.end(), [this](T* descriptor) {
            Descriptor* desc = dynamic_cast<Descriptor*>(descriptor);
            maxFd = std::max(desc->getFd(), maxFd);
        });

        return maxFd;
    }


    void updateFdSet() {
        if (!addedDescriptors.empty() || !removedDescriptors.empty()) {
            for_each(addedDescriptors.begin(), addedDescriptors.end(), [this](T* descriptor) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->getFd(), &fdSet);
                descriptors.push_back(descriptor);
                descriptor->incManaged();
            });
            addedDescriptors.clear();

            for_each(removedDescriptors.begin(), removedDescriptors.end(), [this](T* descriptor) {
                FD_CLR(dynamic_cast<Descriptor*>(descriptor)->getFd(), &fdSet);
                descriptors.remove(descriptor);
                descriptor->decManaged();
            });
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
