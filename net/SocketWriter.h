#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#include "Writer.h"
#include "Socket.h"


class SocketWriter : public Writer, virtual public Socket
{
public:
    void writeEvent();
    
protected:
    SocketWriter() : Socket(), Writer(this->getFd()) {}
    
    SocketWriter(int fd) : Writer(fd) {}
};

#endif // SOCKETWRITER_H
