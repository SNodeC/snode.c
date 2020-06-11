#ifndef BASECONNECTEDSOCKET_H
#define BASECONNECTEDSOCKET_H

#include "SocketConnection.h"


class SocketServer;

template <typename Reader, typename Writer>
class SocketConnectionBase
    : public SocketConnection
    , public Reader
    , public Writer {
public:
    virtual void enqueue(const char* buffer, int size);
    virtual void enqueue(const std::string& junk);
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
