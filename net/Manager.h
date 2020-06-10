#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


template <typename T>
class Manager {
protected:
    Manager()
        : maxFd(0) {
        FD_ZERO(&fdSet);
    }


    virtual ~Manager() {
        descriptors.reverse();
        removedDescriptors = descriptors;
        updateFdSet();
        stashedDescriptors.reverse();
        removedDescriptors = stashedDescriptors;
        updateFdSet();
    }


public:
    fd_set& getFdSet() {
        updateFdSet();

        return fdSet;
    }

    bool contains(std::list<T*>& listOfElements, T*& element) {
        typename std::list<T*>::iterator it = std::find(listOfElements.begin(), listOfElements.end(), element);

        return it != listOfElements.end();
    }

    void add(T* socket) {
        if (!contains(descriptors, socket) && !contains(addedDescriptors, socket)) {
            addedDescriptors.push_back(socket);
        }
    }


    void remove(T* socket) {
        if (contains(descriptors, socket) && !contains(removedDescriptors, socket)) {
            removedDescriptors.push_back(socket);
        }
    }


    void stash(T* socket) {
        addedStashedDescriptors.push_back(socket);
    }


    void unstash(T* socket) {
        removedStashedDescriptors.push_back(socket);
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

        for (T* descriptor : descriptors) {
            Descriptor* desc = dynamic_cast<Descriptor*>(descriptor);
            maxFd = std::max(desc->fd(), maxFd);
        }

        return maxFd;
    }


    void updateFdSet() {
        if (!addedDescriptors.empty() || !removedDescriptors.empty() || !addedStashedDescriptors.empty() ||
            !removedStashedDescriptors.empty()) {
            for (T* descriptor : addedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.push_back(descriptor);
                descriptor->incManaged();
            }
            addedDescriptors.clear();

            for (T* descriptor : removedStashedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.push_back(descriptor);
                stashedDescriptors.remove(descriptor);
                descriptor->decManaged();
            }
            removedStashedDescriptors.clear();

            for (T* descriptor : addedStashedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.remove(descriptor);
                stashedDescriptors.push_back(descriptor);
                descriptor->incManaged();
            }
            addedStashedDescriptors.clear();

            for (T* descriptor : removedDescriptors) {
                FD_CLR(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.remove(descriptor);
                descriptor->decManaged();
            }
            removedDescriptors.clear();

            updateMaxFd();
        }
    }


    fd_set fdSet;
    int maxFd;

    std::list<T*> addedDescriptors;
    std::list<T*> removedDescriptors;
    std::list<T*> addedStashedDescriptors;
    std::list<T*> removedStashedDescriptors;
    std::list<T*> stashedDescriptors;
};

#endif // SOCKETMANAGER_H
