#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#include "Writer.h"
#include "Socket.h"


class SocketWriter : public Writer, virtual public Socket
{
public:
    void writeEvent();
    
protected:
    SocketWriter(int fd, const std::function<void (int errnum)>& onError) : Socket(), Writer(fd, onError) {}
};

#endif // SOCKETWRITER_H
