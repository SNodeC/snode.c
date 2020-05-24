#include "SocketConnection.h"


SocketConnection::SocketConnection(int csFd,
                                   SocketServerInterface* serverSocket,
                                   const std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)>& readProcessor,
                                   const std::function<void (int errnum)>& onReadError,
                                   const std::function<void (int errnum)>& onWriteError
                                  ) :
    Socket(csFd),
    SocketConnectionBase<SocketReader, SocketWriter>(csFd, serverSocket, readProcessor, onReadError, onWriteError) {
}
