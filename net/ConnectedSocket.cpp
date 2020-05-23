#include "ConnectedSocket.h"
#include "Multiplexer.h"
#include "ServerSocket.h"
#include "FileReader.h"


ConnectedSocket::ConnectedSocket(int csFd, 
                                 BaseServerSocket<ConnectedSocket>* serverSocket, 
                                 const std::function<void (ConnectedSocket* cs, const char* junk, ssize_t n)>& readProcessor,
                                 const std::function<void (int errnum)>& onReadError,
                                 const std::function<void (int errnum)>& onWriteError
                                ) 
:   Socket(csFd),
    SocketReader(readProcessor, [&] (int errnum) -> void {
        onReadError(errnum);
    }),
    SocketWriter([&] (int errnum) -> void {
        if (fileReader) {
            fileReader->stop();
            fileReader = 0;
        }
        onWriteError(errnum);
    }),
    serverSocket(serverSocket),
    fileReader(0) {
    }


ConnectedSocket::~ConnectedSocket() {
    serverSocket->disconnect(this);
}


InetAddress& ConnectedSocket::getRemoteAddress() {
    return remoteAddress;
}


void ConnectedSocket::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


void ConnectedSocket::send(const char* puffer, int size) {
    writePuffer.append(puffer, size);
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::send(const std::string& junk) {
    writePuffer += junk;
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::sendFile(const std::string& file, const std::function<void (int ret)>& onError) {
    fileReader = FileReader::read(file,
                    [this] (char* data, int length) -> void {
                        if (length > 0) {
                            this->ConnectedSocket::send(data, length);
                        }
                        fileReader = 0;
                    },
                    [this, onError] (int err) -> void {
                        if (onError) {
                            onError(err);
                        }
                        if (err) {
                            this->end();
                        }
                    });
}


void ConnectedSocket::end() {
    Multiplexer::instance().getReadManager().unmanageSocket(this);
}

