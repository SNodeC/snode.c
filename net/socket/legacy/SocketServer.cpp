#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/SocketServer.h"

namespace legacy {

    SocketServer::SocketServer(
        const std::function<void(SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>* cs)>& onConnect,
        const std::function<void(SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>* cs)>& onDisconnect,
        const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
        const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
        const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError)
        : SocketServerBase<SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>>(onConnect, onDisconnect, readProcessor,
                                                                                             onReadError, onWriteError) {
    }

}; // namespace legacy
