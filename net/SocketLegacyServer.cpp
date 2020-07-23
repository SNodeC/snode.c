#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketLegacyServer.h"


SocketLegacyServer::SocketLegacyServer(const std::function<void (SocketConnection* cs)>& onConnect,
                           const std::function<void (SocketConnection* cs)>& onDisconnect,
                           const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcessor,
                           const std::function<void (int errnum)>& onCsReadError,
                           const std::function<void (int errnum)>& onCsWriteError) :
    SocketServerBase<SocketLegacyConnection>(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError) {
}

                                 
SocketLegacyServer* SocketLegacyServer::instance(const std::function<void (SocketConnection* cs)>& onConnect,
                                     const std::function<void (SocketConnection* cs)>& onDisconnect,
                                     const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcessor,
                                     const std::function<void (int errnum)>& onCsReadError,
                                     const std::function<void (int errnum)>& onCsWriteError) {
    return new SocketLegacyServer(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError);
}

