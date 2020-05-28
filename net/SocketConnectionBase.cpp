#include "SocketConnectionBase.h"

#include "FileReader.h"
#include "Multiplexer.h"
#include "SocketLegacyReader.h"
#include "SocketLegacyWriter.h"
#include "SocketSSLReader.h"
#include "SocketSSLWriter.h"
#include "SocketServer.h"


template <typename R, typename W>
SocketConnectionBase<R, W>::SocketConnectionBase(
    int csFd, SocketServer* serverSocket, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
    const std::function<void(int errnum)>& onReadError, const std::function<void(int errnum)>& onWriteError)
    : R(readProcessor,
        [&](int errnum) -> void {
            onReadError(errnum);
        })
    , W([&](int errnum) -> void {
        if (fileReader) {
            fileReader->stop();
            fileReader = 0;
        }
        onWriteError(errnum);
    })
    , serverSocket(serverSocket)
    , fileReader(0) {
}

template <typename R, typename W> SocketConnectionBase<R, W>::~SocketConnectionBase() {
    serverSocket->disconnect(this);
}

template <typename R, typename W> InetAddress& SocketConnectionBase<R, W>::getRemoteAddress() {
    return remoteAddress;
}

template <typename R, typename W> void SocketConnectionBase<R, W>::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}

template <typename R, typename W> void SocketConnectionBase<R, W>::send(const char* puffer, int size) {
    Writer::writePuffer.append(puffer, size);
    Multiplexer::instance().getManagedWriter().add(this);
}

template <typename R, typename W> void SocketConnectionBase<R, W>::send(const std::string& junk) {
    Writer::writePuffer += junk;
    Multiplexer::instance().getManagedWriter().add(this);
}

template <typename R, typename W>
void SocketConnectionBase<R, W>::sendFile(const std::string& file, const std::function<void(int ret)>& onError) {
    fileReader = FileReader::read(
        file,
        [this](char* data, int length) -> void {
            if (length > 0) {
                this->SocketConnectionBase<R, W>::send(data, length);
            }
            fileReader = 0;
        },
        [this, onError](int err) -> void {
            if (onError) {
                onError(err);
            }
            if (err) {
                this->end();
            }
        });
}

template <typename R, typename W> void SocketConnectionBase<R, W>::end() {
    Multiplexer::instance().getManagedReader().remove(this);
}


template class SocketConnectionBase<SocketLegacyReader, SocketLegacyWriter>;
template class SocketConnectionBase<SocketSSLReader, SocketSSLWriter>;
