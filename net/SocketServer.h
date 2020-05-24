#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServerInterface.h"
#include "SocketReader.h"


class Request;
class Response;
class SocketConnectionInterface;

template<typename T>
class SocketServerBase : public SocketServerInterface, public SocketReader {
private:
    SocketServerBase(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                     const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                     const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcesor,
                     const std::function<void (int errnum)>& onCsReadError,
                     const std::function<void (int errnum)>& onCsWriteError);

public:
    static SocketServerBase* instance(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                                      const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                                      const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcesor,
                                      const std::function<void (int errnum)>& onCsReadError,
                                      const std::function<void (int errnum)>& onCsWriteError);
    ~SocketServerBase() {}

    void listen(in_port_t port, int backlog, const std::function<void (int err)>& onError);

    virtual void readEvent();

    void disconnect(SocketConnectionInterface* cs);

    static void run();

    static void stop();

private:
    std::function<void (SocketConnectionInterface* cs)> onConnect;
    std::function<void (SocketConnectionInterface* cs)> onDisconnect;
    std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)> readProcessor;

    std::function<void (int errnum)> onCsReadError;
    std::function<void (int errnum)> onCsWriteError;
};

#endif // SERVERSOCKET_H
