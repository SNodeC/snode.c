#include <algorithm>
#include "SocketManager.h"
#include "Socket.h"


SocketManager::SocketManager() : maxFd(0) {
    FD_ZERO(&fdSet);
}


SocketManager::~SocketManager() {
}


fd_set& SocketManager::getFdSet() {
    for (std::list<Socket*>::iterator it = addedSockets.begin(); it != addedSockets.end(); ++it) {
        if (std::find(sockets.begin(), sockets.end(), *it) == sockets.end()) {
            FD_SET((*it)->getFd(), &fdSet);
            sockets.push_back(*it);
            (*it)->incManagedCount();
        }
    }
    addedSockets.clear();
    
    for (std::list<Socket*>::iterator it = removedSockets.begin(); it != removedSockets.end(); ++it) {
        if (std::find(sockets.begin(), sockets.end(), *it) != sockets.end()) {
            FD_CLR((*it)->getFd(), &fdSet);
            sockets.remove(*it);
            (*it)->decManagedCount();;
        }
    }
    removedSockets.clear();
    
    updateMaxFd();
    
    return fdSet;
}


void SocketManager::manageSocket(Socket* socket) {
    if (socket != 0 && std::find(addedSockets.begin(), addedSockets.end(), socket) == addedSockets.end()) {
        addedSockets.push_back(socket);
    }
}


void SocketManager::unmanageSocket(Socket* socket) {
    if (socket != 0 && std::find(removedSockets.begin(), removedSockets.end(), socket) == removedSockets.end()) {
        removedSockets.push_back(socket);
    }
}


int SocketManager::updateMaxFd() {
    std::list<Socket*>::iterator it;
    maxFd = 0;
    
    for (it = sockets.begin(); it != sockets.end(); ++it) {
        if ((*it)->getFd() > maxFd) {
            maxFd = (*it)->getFd();
        }
    }
    
    return maxFd;
}


int SocketManager::getMaxFd() {
    return maxFd;
}
