#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServer.h"
#include "SocketLegacyConnection.h"
#include "SocketSSLConnection.h"
#include "SocketLegacyReader.h"


template<typename T>
class SocketServerBase : public SocketServer, public SocketLegacyReader {
protected:
    SocketServerBase(const std::function<void (SocketConnection* cs)>& onConnect,
                     const std::function<void (SocketConnection* cs)>& onDisconnect,
                     const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcesor,
                     const std::function<void (int errnum)>& onCsReadError,
                     const std::function<void (int errnum)>& onCsWriteError);

public:
    virtual ~SocketServerBase() = default;

    void listen(in_port_t port, int backlog, const std::function<void (int err)>& onError);

    virtual void readEvent();

    void disconnect(SocketConnection* cs);

private:
    std::function<void (SocketConnection* cs)> onConnect;
    std::function<void (SocketConnection* cs)> onDisconnect;
    std::function<void (SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;

    std::function<void (int errnum)> onCsReadError;
    std::function<void (int errnum)> onCsWriteError;
};


class SocketSSLServer : public SocketServerBase<SocketSSLConnection> {
private:
    SocketSSLServer(const std::function<void (SocketConnection* cs)>& onConnect,
                     const std::function<void (SocketConnection* cs)>& onDisconnect,
                     const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcesor,
                     const std::function<void (int errnum)>& onCsReadError,
                     const std::function<void (int errnum)>& onCsWriteError);

public:
    static SocketSSLServer* instance(const std::function<void (SocketConnection* cs)>& onConnect,
                                                       const std::function<void (SocketConnection* cs)>& onDisconnect,
                                                       const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcessor,
                                                       const std::function<void (int errnum)>& onCsReadError,
                                                       const std::function<void (int errnum)>& onCsWriteError);
    ~SocketSSLServer();
    
    using SocketServerBase<SocketSSLConnection>::listen;
    void listen(in_port_t port, int backlog, const std::string& cert, const std::string& key, const std::string& password, const std::function<void (int err)>& onError);
    
    virtual void readEvent();
    
private:
    std::function<void (SocketConnection* cs)> onConnect;
    SSL_CTX* ctx;
};


class SocketLegacyServer : public SocketServerBase<SocketLegacyConnection> {
private:
    SocketLegacyServer(const std::function<void (SocketConnection* cs)>& onConnect,
                 const std::function<void (SocketConnection* cs)>& onDisconnect,
                 const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcesor,
                 const std::function<void (int errnum)>& onCsReadError,
                 const std::function<void (int errnum)>& onCsWriteError);
    
public:
    static SocketLegacyServer* instance(const std::function<void (SocketConnection* cs)>& onConnect,
                                  const std::function<void (SocketConnection* cs)>& onDisconnect,
                                  const std::function<void (SocketConnection* cs, const char*  junk, ssize_t n)>& readProcessor,
                                  const std::function<void (int errnum)>& onCsReadError,
                                  const std::function<void (int errnum)>& onCsWriteError);
};


#endif // SERVERSOCKET_H
