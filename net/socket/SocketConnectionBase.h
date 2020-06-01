#ifndef BASECONNECTEDSOCKET_H
#define BASECONNECTEDSOCKET_H

#include "SocketConnection.h"


class SocketServer;

template <typename R, typename W>
class SocketConnectionBase
    : public SocketConnection
    , public R
    , public W {
public:
    void setContext(void* context) {
        this->context = context;
    }

    void* getContext() {
        return this->context;
    }

    virtual void send(const char* puffer, int size);
    virtual void send(const std::string& junk);
    virtual void end();

    virtual InetAddress& getRemoteAddress();
    virtual void setRemoteAddress(const InetAddress& remoteAddress);

private:
    virtual void unmanaged();

protected:
    SocketConnectionBase(int csFd, SocketServer* serverSocket,
                         const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                         const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                         const std::function<void(SocketConnection* cs, int errnum)>& onWriteError);

    SocketServer* serverSocket;

    void* context;

    InetAddress remoteAddress;
};

#endif // BASECONNECTEDSOCKET_H
