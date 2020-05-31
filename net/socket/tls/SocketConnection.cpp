#include "socket/tls/SocketConnection.h"


namespace tls {

    SocketConnection::SocketConnection(int csFd, ::SocketServer* serverSocket,
                                       const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                       const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                                       const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError)
        : tls::Socket(csFd)
        , SocketConnectionBase<tls::SocketReader, tls::SocketWriter>(csFd, serverSocket, readProcessor, onReadError, onWriteError) {
    }

}; // namespace tls
