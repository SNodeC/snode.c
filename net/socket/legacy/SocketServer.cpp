#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/SocketServer.h"

namespace legacy {

    SocketServer::SocketServer(const std::function<void(legacy::SocketConnection* cs)>& onConnect,
                               const std::function<void(legacy::SocketConnection* cs)>& onDisconnect,
                               const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                               const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                               const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError)
        : SocketServerBase<legacy::SocketConnection>(onConnect, onDisconnect, readProcessor, onReadError, onWriteError) {
    }


    SocketServer* SocketServer::instance(const std::function<void(legacy::SocketConnection* cs)>& onConnect,
                                         const std::function<void(legacy::SocketConnection* cs)>& onDisconnect,
                                         const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                         const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                                         const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError) {
        return new SocketServer(onConnect, onDisconnect, readProcessor, onReadError, onWriteError);
    }

}; // namespace legacy
