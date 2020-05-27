#ifndef SSLCONNECTEDSOCKET_H
#define SSLCONNECTEDSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnectionBase.h"
#include "SocketSSLReader.h"
#include "SocketSSLWriter.h"


class SocketServer;

class SocketSSLConnection : public SocketConnectionBase<SocketSSLReader, SocketSSLWriter> {
public:
    SocketSSLConnection(int csFd, SocketServer* ss,
                        const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                        const std::function<void(int errnum)>& onReadError,
                        const std::function<void(int errnum)>& onWriteError);
};

#endif // SSLCONNECTEDSOCKET_H
