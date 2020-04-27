#ifndef SOCKETREADER_H
#define SOCKETREADER_H

#include <functional>
#include <string>

#include "Reader.h"
#include "Socket.h"

class ConnectedSocket;

class SocketReader : public Reader, virtual public Socket
{
public:
    void readEvent();
    void readEvent1();
    
protected:
    SocketReader() : Socket(), Reader(this->getSFd()) {}
    
    SocketReader(int fd) : Reader(fd) {
        this->setSFd(fd);
    }
    
    SocketReader(int fd, std::function<void (ConnectedSocket* cs, std::string line)> rp) : Socket(), Reader(this->getSFd()), readProcessor(rp) {
        this->setSFd(fd);
    }
    
    std::function<void (ConnectedSocket* cs, std::string line)> readProcessor;
};

#endif // SOCKETREADER_H
