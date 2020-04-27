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
    
protected:
    SocketReader() : Socket(), Reader(this->getFd()) {}
    
    SocketReader(int fd) : Reader(fd) {}
    
    SocketReader(int fd, std::function<void (ConnectedSocket* cs, std::string line)> rp) : Socket(), Reader(this->getFd()), readProcessor(rp) {}
    
    std::function<void (ConnectedSocket* cs, std::string line)> readProcessor;
};

#endif // SOCKETREADER_H
