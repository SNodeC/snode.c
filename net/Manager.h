#ifndef MANAGER_H
#define MANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "ManagedDescriptor.h"

class Multiplexer;

template <typename ManagedDescriptor>
class Manager {
public:
    explicit Manager(fd_set& fdSet)
        : fdSet(fdSet) {
    }

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    bool contains(std::list<ManagedDescriptor*>& listOfElements, ManagedDescriptor*& element) const {
        typename std::list<ManagedDescriptor*>::iterator it = std::find(listOfElements.begin(), listOfElements.end(), element);

        return it != listOfElements.end();
    }

    void start(ManagedDescriptor* socket) {
        if (!socket->isManaged() && !contains(addedDescriptors, socket)) {
            addedDescriptors.push_back(socket);
        }
    }

    void stop(ManagedDescriptor* socket) {
        if (socket->isManaged() && !contains(removedDescriptors, socket)) {
            removedDescriptors.push_back(socket);
        }
    }

private:
    int getMaxFd() {
        int fd = 0;

        addDescriptors();
        removeDescriptors();

        if (descriptors.size() > 0) {
            fd = dynamic_cast<Descriptor*>(descriptors.rbegin()->second)->getFd();
        }

        return fd;
    }

    void addDescriptors() {
        if (!addedDescriptors.empty()) {
            for (ManagedDescriptor* descriptor : addedDescriptors) {
                int fd = dynamic_cast<Descriptor*>(descriptor)->getFd();
                FD_SET(fd, &fdSet);
                descriptors[fd] = descriptor;
                descriptor->incManaged();
            }
            addedDescriptors.clear();
        }
    }

    void removeDescriptors() {
        if (!removedDescriptors.empty()) {
            for (ManagedDescriptor* descriptor : removedDescriptors) {
                int fd = dynamic_cast<Descriptor*>(descriptor)->getFd();
                FD_CLR(fd, &fdSet);
                descriptors.erase(fd);
                descriptor->decManaged();
            }
            removedDescriptors.clear();
        }
    }

    void removeManagedDescriptors() {
        for (std::pair<int, ManagedDescriptor*> descriptor : descriptors) {
            removedDescriptors.push_back(descriptor.second);
        }

        removeDescriptors();
    }

protected:
    virtual int dispatch(const fd_set& fdSet, int count) = 0;

    std::map<int, ManagedDescriptor*> descriptors;

private:
    std::list<ManagedDescriptor*> addedDescriptors;
    std::list<ManagedDescriptor*> removedDescriptors;

    fd_set& fdSet;

public:
    using ManagedDescriptorType = ManagedDescriptor;

    friend class Multiplexer;
};

#endif // MANAGER_H
