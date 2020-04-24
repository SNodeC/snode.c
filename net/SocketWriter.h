#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#include "Writer.h"
#include "Socket.h"

/**
 * @todo write docs
 */
class SocketWriter : public virtual Socket, public Writer
{
public:
    SocketWriter() : Socket(), Writer(this->getSFd()) {}
    
    SocketWriter(int fd) : Socket(fd), Writer(fd) {}
    
    void writeEvent();
};

#endif // SOCKETWRITER_H
