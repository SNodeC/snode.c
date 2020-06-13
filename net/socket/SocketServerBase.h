#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServer.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/SocketReader.h"


template <typename SocketConnectionImpl>
class SocketServerBase
    : public SocketServer
    , public Reader
    , public legacy::Socket {
protected:
    SocketServerBase(const std::function<void(SocketConnectionImpl* cs)>& onConnect,
                     const std::function<void(SocketConnectionImpl* cs)>& onDisconnect,
                     const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcesor,
                     const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* cs, int errnum)>& onWriteError);

public:
    virtual ~SocketServerBase() = default;

    void listen(in_port_t port, int backlog, const std::function<void(int err)>& onError) override;

    void readEvent() override;

    void disconnect(SocketConnection* cs) override;

protected:
    void listen(int backlog, const std::function<void(int errnum)>& onError);

private:
    void unmanaged() override;

    std::function<void(SocketConnectionImpl* cs)> onConnect;
    std::function<void(SocketConnectionImpl* cs)> onDisconnect;

    std::function<void(SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;

    std::function<void(SocketConnection* cs, int errnum)> onReadError;
    std::function<void(SocketConnection* cs, int errnum)> onWriteError;
};


#endif // SERVERSOCKET_H
