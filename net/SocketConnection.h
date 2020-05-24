#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "SocketConnectionBase.h"
#include "SocketWriter.h"
#include "SocketReader.h"


class FileReader;
class SocketServerInterface;

class SocketConnection : public SocketConnectionBase<SocketReader, SocketWriter>
{
public:
    SocketConnection(int csFd,
                     SocketServerInterface* ss,
                     const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                     const std::function<void (int errnum)>& onReadError,
                     const std::function<void (int errnum)>& onWriteError
                    );
};

#endif // CONNECTEDSOCKET_H
