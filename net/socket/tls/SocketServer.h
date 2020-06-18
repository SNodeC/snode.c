#ifndef TLS_SOCKETSERVER_H
#define TLS_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnectionBase.h"
#include "socket/SocketServerBase.h"
#include "socket/tls/SocketReader.h"
#include "socket/tls/SocketWriter.h"


namespace tls {

    class SocketServer : public SocketServerBase<SocketConnectionBase<tls::SocketReader, tls::SocketWriter>> {
    private:
        SocketServer(const std::function<void(SocketConnectionBase<tls::SocketReader, tls::SocketWriter>* cs)>& onConnect,
                     const std::function<void(SocketConnectionBase<tls::SocketReader, tls::SocketWriter>* cs)>& onDisconnect,
                     const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);

    public:
        static SocketServer*
        instance(const std::function<void(SocketConnectionBase<tls::SocketReader, tls::SocketWriter>* cs)>& onConnect,
                 const std::function<void(SocketConnectionBase<tls::SocketReader, tls::SocketWriter>* cs)>& onDisconnect,
                 const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                 const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                 const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);
        ~SocketServer() override;

        using SocketServerBase<SocketConnectionBase<tls::SocketReader, tls::SocketWriter>>::listen;
        void listen(in_port_t port, int backlog, const std::string& certChain, const std::string& keyPEM, const std::string& password,
                    const std::function<void(int err)>& onError);

    private:
        std::function<void(SocketConnectionBase<tls::SocketReader, tls::SocketWriter>* cs)> onConnect;
        std::function<void(SocketConnectionBase<tls::SocketReader, tls::SocketWriter>* cs)> onDisconnect;
        SSL_CTX* ctx;
        static int passwordCallback(char* buf, int size, int rwflag, void* u);
    };

}; // namespace tls

#endif // TLS_SOCKETSERVER_H
