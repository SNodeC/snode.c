#ifndef SOCKETREADER_H
#define SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "Socket.h"


class SocketConnection;

class SocketReader : public Reader, virtual public Socket
{
public:
    void readEvent();

protected:
    SocketReader() : readProcessor(0) {}

    SocketReader(const std::function<void (SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor, const std::function<void (int errnum)>& onError) : Socket(), Reader(onError), readProcessor(readProcessor) {}

    std::function<void (SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;
};

#endif // SOCKETREADER_H
