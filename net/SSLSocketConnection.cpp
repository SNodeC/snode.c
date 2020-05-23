#include "SSLSocketConnection.h"
#include "Multiplexer.h"
#include "SocketServer.h"
#include "FileReader.h"


SSLSocketConnection::SSLSocketConnection(int csFd,
                                         SocketServerBase<SSLSocketConnection>* serverSocket,
                                         const std::function<void (SSLSocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                         const std::function<void (int errnum)>& onReadError,
                                         const std::function<void (int errnum)>& onWriteError
                                        ) :
    SSLSocket(csFd),
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


SSLSocketConnection::~SSLSocketConnection() {
    serverSocket->disconnect(this);
}


InetAddress& SSLSocketConnection::getRemoteAddress() {
    return remoteAddress;
}


void SSLSocketConnection::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


void SSLSocketConnection::send(const char* puffer, int size) {
    writePuffer.append(puffer, size);
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void SSLSocketConnection::send(const std::string& junk) {
    writePuffer += junk;
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void SSLSocketConnection::sendFile(const std::string& file, const std::function<void (int ret)>& onError) {
    fileReader = FileReader::read(file,
    [this] (char* data, int length) -> void {
        if (length > 0) {
            this->SSLSocketConnection::send(data, length);
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


void SSLSocketConnection::end() {
    Multiplexer::instance().getReadManager().unmanageSocket(this);
}

