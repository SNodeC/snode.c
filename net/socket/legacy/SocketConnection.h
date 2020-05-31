#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnectionBase.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"


namespace legacy {

    class SocketConnection : public SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter> {
    public:
        SocketConnection(int csFd, ::SocketServer* ss,
                         const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                         const std::function<void(int errnum)>& onReadError, const std::function<void(int errnum)>& onWriteError);
    };

}; // namespace legacy

#endif // CONNECTEDSOCKET_H
