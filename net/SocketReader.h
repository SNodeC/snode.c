#ifndef SOCKETREADER_H
#define SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "SSLSocket.h"


class ConnectedSocket;

class SocketReader : public Reader, virtual public SSLSocket
{
public:
    void readEvent();
    
protected:
    SocketReader() : readProcessor(0) {}
    
    SocketReader(const std::function<void (ConnectedSocket* cs, const char* junk, ssize_t n)>& readProcessor, const std::function<void (int errnum)>& onError) : SSLSocket(), Reader(onError), readProcessor(readProcessor) {}
    
    std::function<void (ConnectedSocket* cs, const char* junk, ssize_t n)> readProcessor;
};

#endif // SOCKETREADER_H
