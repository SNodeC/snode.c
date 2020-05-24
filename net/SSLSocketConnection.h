#ifndef SSLCONNECTEDSOCKET_H
#define SSLCONNECTEDSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnectionBase.h"
#include "SSLSocketWriter.h"
#include "SSLSocketReader.h"


class SocketServerInterface;

class SSLSocketConnection : public SocketConnectionBase<SSLSocketReader, SSLSocketWriter>
{
public:
    SSLSocketConnection(int csFd,
                        SocketServerInterface* ss,
                        const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                        const std::function<void (int errnum)>& onReadError,
                        const std::function<void (int errnum)>& onWriteError
                       );
};

#endif // SSLCONNECTEDSOCKET_H
