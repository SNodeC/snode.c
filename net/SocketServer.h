#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServerInterface.h"
#include "SocketConnection.h"
#include "SSLSocketConnection.h"
#include "SocketReader.h"


template<typename T>
class SocketServerBase : public SocketServerInterface, public SocketReader {
protected:
    SocketServerBase(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                     const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                     const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcesor,
                     const std::function<void (int errnum)>& onCsReadError,
                     const std::function<void (int errnum)>& onCsWriteError);

public:
    virtual ~SocketServerBase() = default;

    void listen(in_port_t port, int backlog, const std::function<void (int err)>& onError);

    virtual void readEvent();

    void disconnect(SocketConnectionInterface* cs);

private:
    std::function<void (SocketConnectionInterface* cs)> onConnect;
    std::function<void (SocketConnectionInterface* cs)> onDisconnect;
    std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)> readProcessor;

    std::function<void (int errnum)> onCsReadError;
    std::function<void (int errnum)> onCsWriteError;
};


class SSLSocketServer : public SocketServerBase<SSLSocketConnection> {
private:
    SSLSocketServer(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                     const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                     const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcesor,
                     const std::function<void (int errnum)>& onCsReadError,
                     const std::function<void (int errnum)>& onCsWriteError);

public:
    static SSLSocketServer* instance(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                                                       const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                                                       const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                                                       const std::function<void (int errnum)>& onCsReadError,
                                                       const std::function<void (int errnum)>& onCsWriteError);
    ~SSLSocketServer();
    
    using SocketServerBase<SSLSocketConnection>::listen;
    void listen(in_port_t port, int backlog, const std::string& cert, const std::string& key, const std::string& password, const std::function<void (int err)>& onError);
    
    virtual void readEvent();
    
private:
    std::function<void (SocketConnectionInterface* cs)> onConnect;
    SSL_CTX* ctx;
};


class SocketServer : public SocketServerBase<SocketConnection> {
private:
    SocketServer(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                 const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                 const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcesor,
                 const std::function<void (int errnum)>& onCsReadError,
                 const std::function<void (int errnum)>& onCsWriteError);
    
public:
    static SocketServer* instance(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                                  const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                                  const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                                  const std::function<void (int errnum)>& onCsReadError,
                                  const std::function<void (int errnum)>& onCsWriteError);
};


#endif // SERVERSOCKET_H
