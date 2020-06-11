#include "SocketConnectionBase.h"

#include "Multiplexer.h"
#include "SocketServer.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"
#include "socket/tls/SocketReader.h"
#include "socket/tls/SocketWriter.h"


template <typename Reader, typename Writer>
SocketConnectionBase<Reader, Writer>::SocketConnectionBase(
    int csFd, SocketServer* serverSocket, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
    const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
    const std::function<void(SocketConnection* cs, int errnum)>& onWriteError)
    : Reader(readProcessor,
             [&](int errnum) -> void {
                 onReadError(this, errnum);
             })
    , Writer([&](int errnum) -> void {
        onWriteError(this, errnum);
    })
    , serverSocket(serverSocket) {
    this->attachFd(csFd);
}


template <typename Reader, typename Writer>
InetAddress& SocketConnectionBase<Reader, Writer>::getRemoteAddress() {
    return remoteAddress;
}


template <typename Reader, typename Writer>
void SocketConnectionBase<Reader, Writer>::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


template <typename Reader, typename Writer>
void SocketConnectionBase<Reader, Writer>::enqueue(const char* buffer, int size) {
    Writer::writePuffer.append(buffer, size);
    Multiplexer::instance().getManagedWriter().add(this);
}


template <typename Reader, typename Writer>
void SocketConnectionBase<Reader, Writer>::enqueue(const std::string& junk) {
    enqueue(junk.c_str(), junk.size());
}


template <typename Reader, typename Writer>
void SocketConnectionBase<Reader, Writer>::end() {
    Multiplexer::instance().getManagedReader().remove(this);
}


template <typename Reader, typename Writer>
void SocketConnectionBase<Reader, Writer>::unmanaged() {
    serverSocket->disconnect(this);
}


template class SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>;
template class SocketConnectionBase<tls::SocketReader, tls::SocketWriter>;
