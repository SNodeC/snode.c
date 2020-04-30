#include <iostream>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "SocketReader.h"
#include "Multiplexer.h"

#include "ConnectedSocket.h"


void SocketReader::readEvent() {
    #define MAX_JUNKSIZE 4096
    static char junk[MAX_JUNKSIZE];
    
    ssize_t ret = recv(this->getFd(), junk, MAX_JUNKSIZE, 0);
    
    if (ret > 0) {
        readProcessor(dynamic_cast<ConnectedSocket*>(this), junk, ret);
    } else if (ret == 0) {
        std::cout << "EOF: " << this->getFd() << std::endl;
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    } else {
        std::cout << "Read: " << strerror(errno) << std::endl;
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    }
}

