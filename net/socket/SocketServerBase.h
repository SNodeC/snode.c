#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServer.h"
#include "socket/legacy/SocketReader.h"


template <typename T>
class SocketServerBase
    : public SocketServer
    , public legacy::SocketReader {
protected:
    SocketServerBase(const std::function<void(SocketConnection* cs)>& onConnect,
                     const std::function<void(SocketConnection* cs)>& onDisconnect,
                     const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcesor,
                     const std::function<void(SocketConnection* cs, int errnum)>& onCsReadError,
                     const std::function<void(SocketConnection* cs, int errnum)>& onCsWriteError);

public:
    virtual ~SocketServerBase() = default;

    void listen(in_port_t port, int backlog, const std::function<void(int err)>& onError);

    virtual void readEvent();

    void disconnect(SocketConnection* cs);

protected:
    void listen(int backlog, const std::function<void(int errnum)>& onError);

private:
    virtual void unmanaged();

    std::function<void(SocketConnection* cs)> onConnect;
    std::function<void(SocketConnection* cs)> onDisconnect;
    std::function<void(SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;

    std::function<void(SocketConnection* cs, int errnum)> onCsReadError;
    std::function<void(SocketConnection* cs, int errnum)> onCsWriteError;
};


#endif // SERVERSOCKET_H
