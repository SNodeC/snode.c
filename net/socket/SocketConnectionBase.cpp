#include "SocketConnectionBase.h"

#include "Multiplexer.h"
#include "SocketServer.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"
#include "socket/tls/SocketReader.h"
#include "socket/tls/SocketWriter.h"


template <typename R, typename W>
SocketConnectionBase<R, W>::SocketConnectionBase(
    int csFd, SocketServer* serverSocket, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
    const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
    const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError)
    : R(readProcessor,
        [&](int errnum) -> void {
            onReadError(this, errnum);
        })
    , W([&](int errnum) -> void {
        onWriteError(this, errnum);
    })
    , serverSocket(serverSocket) {
}


template <typename R, typename W>
InetAddress& SocketConnectionBase<R, W>::getRemoteAddress() {
    return remoteAddress;
}


template <typename R, typename W>
void SocketConnectionBase<R, W>::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


template <typename R, typename W>
void SocketConnectionBase<R, W>::enqueue(const char* buffer, int size) {
    Writer::writePuffer.append(buffer, size);
    Multiplexer::instance().getManagedWriter().add(this);
}


template <typename R, typename W>
void SocketConnectionBase<R, W>::enqueue(const std::string& junk) {
    enqueue(junk.c_str(), junk.size());
}


template <typename R, typename W>
void SocketConnectionBase<R, W>::end() {
    Multiplexer::instance().getManagedReader().remove(this);
}


template <typename R, typename W>
void SocketConnectionBase<R, W>::unmanaged() {
    serverSocket->disconnect(this);
}


template class SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>;
template class SocketConnectionBase<tls::SocketReader, tls::SocketWriter>;
