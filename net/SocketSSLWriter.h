#ifndef SSLSOCKETWRITER_H
#define SSLSOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"
#include "SocketSSL.h"


class SocketSSLWriter : public Writer, virtual public SocketSSL
{
public:
    void writeEvent();

protected:
    SocketSSLWriter(const std::function<void (int errnum)>& onError) : Writer(onError) {}
};

#endif // SSLSOCKETWRITER_H
