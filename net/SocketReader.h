#ifndef SOCKETREADER_H
#define SOCKETREADER_H

#include "Reader.h"
#include "Socket.h"


class SocketReader : public Reader, virtual public Socket
{
public:
    void readEvent();
    
protected:
    SocketReader() : Socket(), Reader(this->getSFd()) {}
    
    SocketReader(int fd) : Reader(fd) {
        this->setSFd(fd);
    }
};

#endif // SOCKETREADER_H
