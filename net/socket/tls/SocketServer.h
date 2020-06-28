#ifndef TLS_SOCKETSERVER_H
#define TLS_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnectionBase.h"
#include "socket/SocketServerBase.h"
#include "socket/tls/SocketReader.h"
#include "socket/tls/SocketWriter.h"


namespace tls {

    using SocketConnection = SocketConnectionBase<tls::SocketReader, tls::SocketWriter>;

    class SocketServer : public SocketServerBase<tls::SocketConnection> {
    private:
        SocketServer(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                     const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                     const std::function<void(SocketReaderBase* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);

    public:
        static SocketServer* instance(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                                      const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                                      const std::function<void(SocketReaderBase* cs, const char* junk, ssize_t n)>& readProcessor,
                                      const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                                      const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);
        ~SocketServer() override;

    protected:
        using SocketServerBase<tls::SocketConnection>::listen;

    public:
        void listen(in_port_t port, int backlog, const std::string& certChain, const std::string& keyPEM, const std::string& password,
                    const std::function<void(int err)>& onError);

    private:
        std::function<void(tls::SocketConnection* cs)> onConnect;
        std::function<void(tls::SocketConnection* cs)> onDisconnect;
        SSL_CTX* ctx;
        static int passwordCallback(char* buf, int size, int rwflag, void* u);
    };

}; // namespace tls

#endif // TLS_SOCKETSERVER_H
