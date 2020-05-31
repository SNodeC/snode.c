#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/SocketServer.h"

namespace legacy {

    SocketServer::SocketServer(const std::function<void(::SocketConnection* cs)>& onConnect,
                               const std::function<void(::SocketConnection* cs)>& onDisconnect,
                               const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                               const std::function<void(::SocketConnection* cs, int errnum)>& onCsReadError,
                               const std::function<void(::SocketConnection* cs, int errnum)>& onCsWriteError)
        : SocketServerBase<legacy::SocketConnection>(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError) {
    }


    SocketServer* SocketServer::instance(const std::function<void(::SocketConnection* cs)>& onConnect,
                                         const std::function<void(::SocketConnection* cs)>& onDisconnect,
                                         const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                         const std::function<void(::SocketConnection* cs, int errnum)>& onCsReadError,
                                         const std::function<void(::SocketConnection* cs, int errnum)>& onCsWriteError) {
        return new SocketServer(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError);
    }

}; // namespace legacy
