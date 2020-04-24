#ifndef SOCKETREADER_H
#define SOCKETREADER_H

#include "Reader.h"
#include "Socket.h"

class SocketReader : virtual public Socket, public Reader
{
protected:
    SocketReader() : Socket(), Reader(this->getSFd()) {}
    
    SocketReader(int fd) : Socket(fd), Reader(fd) {}
    
    void readEvent();
};

#endif // SOCKETREADER_H
