#ifndef SSLSOCKETREADER_H
#define SSLSOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "SocketSSL.h"


class SocketConnection;

class SocketSSLReader : public Reader, virtual public SocketSSL
{
public:
    void readEvent();

protected:
    SocketSSLReader() : readProcessor(0) {}

    SocketSSLReader(const std::function<void (SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor, const std::function<void (int errnum)>& onError) : SocketSSL(), Reader(onError), readProcessor(readProcessor) {}

    std::function<void (SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;
};

#endif // SSLSOCKETREADER_H
