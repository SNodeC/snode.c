#ifndef SSLSOCKETREADER_H
#define SSLSOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "SSLSocket.h"


class SocketConnectionInterface;

class SSLSocketReader : public Reader, virtual public SSLSocket
{
public:
    void readEvent();

protected:
    SSLSocketReader() : readProcessor(0) {}

    SSLSocketReader(const std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)>& readProcessor, const std::function<void (int errnum)>& onError) : SSLSocket(), Reader(onError), readProcessor(readProcessor) {}

    std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)> readProcessor;
};

#endif // SSLSOCKETREADER_H
