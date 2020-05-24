#include "SSLSocketConnection.h"


SSLSocketConnection::SSLSocketConnection(int csFd,
                                         SocketServerInterface* serverSocket,
                                         const std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)>& readProcessor,
                                         const std::function<void (int errnum)>& onReadError,
                                         const std::function<void (int errnum)>& onWriteError
                                        ) :
    SSLSocket(csFd),
    SocketConnectionBase<SSLSocketReader, SSLSocketWriter>(csFd, serverSocket, readProcessor, onReadError, onWriteError)  {
}
