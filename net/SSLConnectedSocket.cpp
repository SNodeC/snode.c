#include "SSLConnectedSocket.h"
#include "Multiplexer.h"
#include "ServerSocket.h"
#include "FileReader.h"


SSLConnectedSocket::SSLConnectedSocket(int csFd, 
                                 ServerSocket* serverSocket, 
                                 const std::function<void (SSLConnectedSocket* cs, const char* junk, ssize_t n)>& readProcessor,
                                 const std::function<void (int errnum)>& onReadError,
                                 const std::function<void (int errnum)>& onWriteError
                                ) 
:   SSLSocket(csFd),
    SSLSocketReader(readProcessor, [&] (int errnum) -> void {
        onReadError(errnum);
    }),
    SSLSocketWriter([&] (int errnum) -> void {
        if (fileReader) {
            fileReader->stop();
            fileReader = 0;
        }
        onWriteError(errnum);
    }),
    serverSocket(serverSocket),
    fileReader(0) {
}


SSLConnectedSocket::~SSLConnectedSocket() {
    serverSocket->disconnect(this);
}


InetAddress& SSLConnectedSocket::getRemoteAddress() {
    return remoteAddress;
}


void SSLConnectedSocket::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


void SSLConnectedSocket::send(const char* puffer, int size) {
    writePuffer.append(puffer, size);
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void SSLConnectedSocket::send(const std::string& junk) {
    writePuffer += junk;
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void SSLConnectedSocket::sendFile(const std::string& file, const std::function<void (int ret)>& onError) {
    fileReader = FileReader::read(file,
                    [this] (char* data, int length) -> void {
                        if (length > 0) {
                            this->SSLConnectedSocket::send(data, length);
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


void SSLConnectedSocket::end() {
    Multiplexer::instance().getReadManager().unmanageSocket(this);
}

