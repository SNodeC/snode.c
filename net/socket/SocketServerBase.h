#ifndef SOCKETSERVERBASE_H
#define SOCKETSERVERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "Socket.h"
#include "SocketServer.h"


template <typename SocketConnectionImpl>
class SocketServerBase
    : public SocketServer
    , public Reader
    , public Socket {
protected:
    SocketServerBase(const std::function<void(SocketConnectionImpl* cs)>& onConnect,
                     const std::function<void(SocketConnectionImpl* cs)>& onDisconnect,
                     const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* cs, int errnum)>& onWriteError);

public:
    ~SocketServerBase() override = default;

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


#endif // SOCKETSERVERBASE_H
