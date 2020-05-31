#include "socket/legacy/SocketConnection.h"


namespace legacy {

    SocketConnection::SocketConnection(int csFd, ::SocketServer* serverSocket,
                                       const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                       const std::function<void(int errnum)>& onReadError,
                                       const std::function<void(int errnum)>& onWriteError)
        : legacy::Socket(csFd)
        , SocketConnectionBase<SocketReader, SocketWriter>(csFd, serverSocket, readProcessor, onReadError, onWriteError) {
    }

}; // namespace legacy
