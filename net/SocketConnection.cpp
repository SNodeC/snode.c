#include "SocketConnection.h"
#include "Multiplexer.h"
#include "SocketServer.h"
#include "FileReader.h"


SocketConnection::SocketConnection(int csFd,
                                   SocketServerBase<SocketConnection>* serverSocket,
                                   const std::function<void (SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
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


SocketConnection::~SocketConnection() {
    serverSocket->disconnect(this);
}


InetAddress& SocketConnection::getRemoteAddress() {
    return remoteAddress;
}


void SocketConnection::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


void SocketConnection::send(const char* puffer, int size) {
    writePuffer.append(puffer, size);
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void SocketConnection::send(const std::string& junk) {
    writePuffer += junk;
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void SocketConnection::sendFile(const std::string& file, const std::function<void (int ret)>& onError) {
    fileReader = FileReader::read(file,
    [this] (char* data, int length) -> void {
        if (length > 0) {
            this->SocketConnection::send(data, length);
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


void SocketConnection::end() {
    Multiplexer::instance().getReadManager().unmanageSocket(this);
}

