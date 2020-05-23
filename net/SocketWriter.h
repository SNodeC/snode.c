#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#include <functional>

#include "Writer.h"
#include "Socket.h"


class SocketWriter : public Writer, virtual public Socket
{
public:
    void writeEvent();
    
protected:
    SocketWriter(const std::function<void (int errnum)>& onError) : Writer(onError) {}
};

#endif // SOCKETWRITER_H
