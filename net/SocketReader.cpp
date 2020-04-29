#include <iostream>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "SocketReader.h"
#include "Multiplexer.h"

#include "ConnectedSocket.h"


void SocketReader::readEvent() {
    #define MAX_JUNKSIZE 4096
    char junk[MAX_JUNKSIZE];
    
    ssize_t ret = recv(dynamic_cast<Descriptor*>(this)->getFd(), junk, MAX_JUNKSIZE, 0);
    
    if (ret > 0) {
        std::string line(junk, ret);
        readProcessor(dynamic_cast<ConnectedSocket*>(this), line);
    } else if (ret == 0) {
        std::cout << "EOF: " << dynamic_cast<Descriptor*>(this)->getFd() << std::endl;
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    } else {
        std::cout << "Read: " << strerror(errno) << std::endl;
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    }
}

