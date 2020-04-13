#include <sys/types.h>
#include <sys/socket.h>

#include "SocketWriteManager.h"
#include "ConnectedSocket.h"


int SocketWriteManager::process(fd_set& fdSet, int count) {
    for (std::list<Socket*>::iterator it = sockets.begin(); it != sockets.end() && count > 0; ++it) {
        if (FD_ISSET((*it)->getFd(), &fdSet)) {
            count--;
            if (ConnectedSocket* connectedSocket = dynamic_cast<ConnectedSocket*> (*it)) {
                connectedSocket->send();
            }
        }
    }
                
    return count;
}
