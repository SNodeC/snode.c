#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#include <functional>

#include "Writer.h"
#include "SSLSocket.h"


class SSLSocketWriter : public Writer, virtual public SSLSocket
{
public:
    void writeEvent();
    
protected:
    SSLSocketWriter(const std::function<void (int errnum)>& onError) : Writer(onError) {}
};

#endif // SOCKETWRITER_H
