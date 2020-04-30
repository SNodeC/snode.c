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
    SocketReader() : Socket(), Reader(0), readProcessor(0) {}
    
    SocketReader(int fd, const std::function<void (ConnectedSocket* cs, const char* junk, ssize_t n)>& rp) : Socket(), Reader(fd), readProcessor(rp) {}
    
    std::function<void (ConnectedSocket* cs, const char* junk, ssize_t n)> readProcessor;
};

#endif // SOCKETREADER_H
