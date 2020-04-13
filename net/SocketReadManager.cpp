#include <unistd.h>

#include <iostream>
#include <functional>

#include "SocketReadManager.h"
#include "ServerSocket.h"
#include "ConnectedSocket.h"

#define LAMBDACB(ret, name, args) std::function<ret (args)> name
#define LAMBDA(ret, args) [&] args -> ret

void readJunk(int fd, 
              LAMBDACB(void, JUNKcb, (const char* junk, int ret)),
              LAMBDACB(void, EOFcb, ()),
              LAMBDACB(void, ERRcb, (int err))) {
#define MAX_JUNKSIZE 4096
    char junk[MAX_JUNKSIZE - 1];
    
    int ret = recv(fd, junk, MAX_JUNKSIZE, 0);
    
    if (ret > 0) {
        junk[ret] = 0;
        JUNKcb(junk, ret);
    } else if (ret == 0) {
        EOFcb();
    } else {
        ERRcb(ret);
    }
}

int SocketReadManager::process(fd_set& fdSet, int count) {
    for (std::list<Socket*>::iterator it = sockets.begin(); it != sockets.end() && count > 0; ++it) {
        if (FD_ISSET((*it)->getFd(), &fdSet)) {
            count--;
            if (ServerSocket* serverSocket = dynamic_cast<ServerSocket*> (*it)) {
                ConnectedSocket* cs = serverSocket->accept();
                this->manageSocket(cs);
            } else if (ConnectedSocket* connectedSocket = dynamic_cast<ConnectedSocket*> (*it)) {
                readJunk(connectedSocket->getFd(),
                         LAMBDA(void, (const char* junk, int n)) {
                             connectedSocket->push(junk, n);
                         },
                         LAMBDA(void, ()) {
                             std::cout << "EOF: " << connectedSocket->getFd() << std::endl;
                             this->unmanageSocket(connectedSocket);
                         },
                         LAMBDA(void, (int err)) {
                             std::cout << "ERR " << err << std::endl;
                             this->unmanageSocket(connectedSocket);
                         });
            }
        }
    }
    
    return count;
}
