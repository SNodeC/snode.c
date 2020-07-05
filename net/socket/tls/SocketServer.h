#ifndef TLS_SOCKETSERVER_H
#define TLS_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketServer.h"
#include "socket/tls/SocketConnection.h"


namespace tls {

    class SocketServer : public ::SocketServer<tls::SocketConnection> {
    private:
        SocketServer(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                     const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                     const std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(tls::SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(tls::SocketConnection* cs, int errnum)>& onWriteError);

    public:
        static SocketServer* instance(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                                      const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                                      const std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                                      const std::function<void(tls::SocketConnection* cs, int errnum)>& onReadError,
                                      const std::function<void(tls::SocketConnection* cs, int errnum)>& onWriteError);

        ~SocketServer() override;

    protected:
        using ::SocketServer<tls::SocketConnection>::listen;

    public:
        void listen(in_port_t port, int backlog, const std::string& certChain, const std::string& keyPEM, const std::string& password,
                    const std::function<void(int err)>& onError);

    private:
        SSL_CTX* ctx;
        static int passwordCallback(char* buf, int size, int rwflag, void* u);
    };

}; // namespace tls

#endif // TLS_SOCKETSERVER_H
