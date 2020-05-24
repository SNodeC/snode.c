#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnectionBase.h"
#include "SocketLegacyReader.h"
#include "SocketLegacyWriter.h"


class SocketServer;

class SocketLegacyConnection : public SocketConnectionBase<SocketLegacyReader, SocketLegacyWriter>
{
public:
    SocketLegacyConnection(int csFd,
                           SocketServer* ss,
                     const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                     const std::function<void (int errnum)>& onReadError,
                     const std::function<void (int errnum)>& onWriteError
                    );
};

#endif // CONNECTEDSOCKET_H
