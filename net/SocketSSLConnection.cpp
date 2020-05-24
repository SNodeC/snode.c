#include "SocketSSLConnection.h"


SocketSSLConnection::SocketSSLConnection(int csFd,
                                         SocketServer* serverSocket,
                                         const std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)>& readProcessor,
                                         const std::function<void (int errnum)>& onReadError,
                                         const std::function<void (int errnum)>& onWriteError
                                        ) :
        SocketSSL(csFd),
        SocketConnectionBase<SocketSSLReader, SocketSSLWriter>(csFd, serverSocket, readProcessor, onReadError, onWriteError)  {
}
