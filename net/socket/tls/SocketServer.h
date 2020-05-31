#ifndef SOCKETSSLSERVER_H
#define SOCKETSSLSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketServerBase.h"
#include "socket/tls/SocketConnection.h"


namespace tls {

    class SocketServer : public SocketServerBase<tls::SocketConnection> {
    private:
        SocketServer(const std::function<void(::SocketConnection* cs)>& onConnect,
                     const std::function<void(::SocketConnection* cs)>& onDisconnect,
                     const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcesor,
                     const std::function<void(int errnum)>& onCsReadError, const std::function<void(int errnum)>& onCsWriteError);

    public:
        static SocketServer* instance(const std::function<void(::SocketConnection* cs)>& onConnect,
                                      const std::function<void(::SocketConnection* cs)>& onDisconnect,
                                      const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                      const std::function<void(int errnum)>& onCsReadError,
                                      const std::function<void(int errnum)>& onCsWriteError);
        ~SocketServer();

        using SocketServerBase<tls::SocketConnection>::listen;
        void listen(in_port_t port, int backlog, const std::string& certChain, const std::string& keyPEM, const std::string& password,
                    const std::function<void(int err)>& onError);

    private:
        std::function<void(::SocketConnection* cs)> onConnect;
        SSL_CTX* ctx;
        static int passwordCallback(char* buf, int size, int rwflag, void* u);
    };

}; // namespace tls

#endif // SOCKETSSLSERVER_H
