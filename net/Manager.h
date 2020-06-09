#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <iostream>
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
            std::cout << "ADD: " << dynamic_cast<Descriptor*>(socket)->fd() << std::endl;
            //        if (!socket->managed) {   // need a search
            addedDescriptors.push_back(socket);
        }
    }


    void remove(T* socket) {
        if (contains(descriptors, socket) && !contains(removedDescriptors, socket)) {
            std::cout << "REMOVE: " << dynamic_cast<Descriptor*>(socket)->fd() << std::endl;
            //        if (socket->managed) {
            removedDescriptors.push_back(socket);
        }
    }


    void stash(T* socket) {
        std::cout << "STASH: " << dynamic_cast<Descriptor*>(socket)->fd() << std::endl;
        //        if (socket->managed) {
//        removedDescriptors.push_back(socket);
        stashedDescriptors.push_back(socket);
//        socket->incManaged();
        //        }
    }


    void unstash(T* socket) {
        
        std::cout << "UNSTASH: " << dynamic_cast<Descriptor*>(socket)->fd() << std::endl;
        //        if (socket->managed) {
        unstashedDescriptors.push_back(socket);
//        stashedDescriptors.remove(socket);
        //        }
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
        if (!addedDescriptors.empty() || !removedDescriptors.empty() || !stashedDescriptors.empty() || !unstashedDescriptors.empty()) {
            for (T* descriptor : addedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.push_back(descriptor);
                descriptor->incManaged();
                descriptor->checkForEOF();
                
            }
            addedDescriptors.clear();

            for (T* descriptor : stashedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.remove(descriptor);
                descriptor->incManaged();
                descriptor->checkForEOF();
            }
            stashedDescriptors.clear();
            
            for (T* descriptor : removedDescriptors) {
                FD_CLR(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.remove(descriptor);
                descriptor->decManaged();
                descriptor->checkForEOF();
            }
            removedDescriptors.clear();

            for (T* descriptor : unstashedDescriptors) {
                FD_SET(dynamic_cast<Descriptor*>(descriptor)->fd(), &fdSet);
                descriptors.push_back(descriptor);
                descriptor->decManaged();
                descriptor->checkForEOF();
            }
            unstashedDescriptors.clear();

            updateMaxFd();
        }
    }


    fd_set fdSet;
    int maxFd;

    std::list<T*> addedDescriptors;
    std::list<T*> removedDescriptors;
    std::list<T*> stashedDescriptors;
    std::list<T*> unstashedDescriptors;
};

#endif // SOCKETMANAGER_H
