#include "SocketLegacyConnection.h"


namespace legacy {

SocketLegacyConnection::SocketLegacyConnection(int csFd, SocketServer* serverSocket,
                                               const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                               const std::function<void(int errnum)>& onReadError,
                                               const std::function<void(int errnum)>& onWriteError)
    : SocketLegacy(csFd)
    , SocketConnectionBase<SocketLegacyReader, SocketLegacyWriter>(csFd, serverSocket, readProcessor, onReadError, onWriteError) {
}

};
