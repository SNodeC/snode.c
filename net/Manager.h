#ifndef MANAGER_H
#define MANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


template <typename ManagedDescriptor>
class Manager {
protected:
    Manager() = default;

public:
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    virtual ~Manager() {
        stop();
    }


    void stop() {
        updateFdSet();

        removedDescriptors = descriptors;
        updateFdSet();

        removedDescriptors = stashedDescriptors;
        updateFdSet();
    }


    fd_set& getFdSet() {
        updateFdSet();

        return fdSet;
    }

    bool contains(std::list<ManagedDescriptor*>& listOfElements, ManagedDescriptor*& element) {
        typename std::list<ManagedDescriptor*>::iterator it = std::find(listOfElements.begin(), listOfElements.end(), element);

        return it != listOfElements.end();
    }

    void add(ManagedDescriptor* socket) {
        if (!contains(descriptors, socket) && !contains(addedDescriptors, socket)) {
            addedDescriptors.push_back(socket);
        }
    }


    void remove(ManagedDescriptor* socket) {
        if (contains(descriptors, socket) && !contains(removedDescriptors, socket)) {
            removedDescriptors.push_back(socket);
        }
    }


    void stash(ManagedDescriptor* socket) {
        if (contains(descriptors, socket) && !contains(addedStashedDescriptors, socket)) {
            addedStashedDescriptors.push_back(socket);
        }
    }


    void unstash(ManagedDescriptor* socket) {
        if (!contains(descriptors, socket) && !contains(untashedDescriptors, socket)) {
            untashedDescriptors.push_back(socket);
        }
    }


    int getMaxFd() {
        return maxFd;
    }


    virtual int dispatch(const fd_set& fdSet, int count) = 0;

protected:
    std::list<ManagedDescriptor*> descriptors;


private:
    int updateMaxFd() {
        maxFd = 0;

        for (ManagedDescriptor* descriptor : descriptors) {
            Descriptor* desc = dynamic_cast<Descriptor*>(descriptor);
            maxFd = std::max(desc->getFd(), maxFd);
        }

        return maxFd;
    }


    void updateFdSet() {
        if (!addedDescriptors.empty() || !removedDescriptors.empty() || !addedStashedDescriptors.empty() ||
            !untashedDescriptors.empty()) {
            for (ManagedDescriptor* descriptor : addedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->getFd(), &fdSet);
                descriptors.push_back(descriptor);
                descriptor->incManaged();
            }
            addedDescriptors.clear();

            for (ManagedDescriptor* descriptor : untashedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->getFd(), &fdSet);
                descriptors.push_back(descriptor);
                stashedDescriptors.remove(descriptor);
                descriptor->decManaged();
            }
            untashedDescriptors.clear();

            for (ManagedDescriptor* descriptor : addedStashedDescriptors) {
                FD_CLR(dynamic_cast<Descriptor*>(descriptor)->getFd(), &fdSet);
                descriptors.remove(descriptor);
                stashedDescriptors.push_back(descriptor);
                descriptor->incManaged();
            }
            addedStashedDescriptors.clear();

            for (ManagedDescriptor* descriptor : removedDescriptors) {
                FD_CLR(dynamic_cast<Descriptor*>(descriptor)->getFd(), &fdSet);
                descriptors.remove(descriptor);
                descriptor->decManaged();
            }
            removedDescriptors.clear();

            updateMaxFd();
        }
    }


    fd_set fdSet{0};
    int maxFd{0};

    std::list<ManagedDescriptor*> addedDescriptors;
    std::list<ManagedDescriptor*> removedDescriptors;
    std::list<ManagedDescriptor*> addedStashedDescriptors;
    std::list<ManagedDescriptor*> untashedDescriptors;
    std::list<ManagedDescriptor*> stashedDescriptors;
};

#endif // MANAGER_H
