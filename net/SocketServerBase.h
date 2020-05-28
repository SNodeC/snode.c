#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketLegacyReader.h"
#include "SocketServer.h"


template <typename T>
class SocketServerBase
    : public SocketServer
    , public SocketLegacyReader {
protected:
    SocketServerBase(const std::function<void(SocketConnection* cs)>& onConnect,
                     const std::function<void(SocketConnection* cs)>& onDisconnect,
                     const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcesor,
                     const std::function<void(int errnum)>& onCsReadError, const std::function<void(int errnum)>& onCsWriteError);

public:
    virtual ~SocketServerBase() = default;

    void listen(in_port_t port, int backlog, const std::function<void(int err)>& onError);

    virtual void readEvent();

    void disconnect(SocketConnection* cs);

private:
    std::function<void(SocketConnection* cs)> onConnect;
    std::function<void(SocketConnection* cs)> onDisconnect;
    std::function<void(SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;

    std::function<void(int errnum)> onCsReadError;
    std::function<void(int errnum)> onCsWriteError;
};


#endif // SERVERSOCKET_H
