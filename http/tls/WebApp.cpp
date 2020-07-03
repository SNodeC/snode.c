#include "WebApp.h"

#include "HTTPContext.h"
#include "socket/tls/SocketServer.h"


namespace tls {

    void WebApp::listen(int port, const std::function<void(int err)>& onError) {
        errno = 0;

        tls::SocketServer::instance(
            [this](tls::SocketConnection* connectedSocket) -> void { // onConnect
                connectedSocket->setContext(new HTTPContext(*this, connectedSocket));
            },
            [](tls::SocketConnection* connectedSocket) -> void { // onDisconnect
                delete static_cast<HTTPContext*>(connectedSocket->getContext());
            },
            [](tls::SocketConnection* connectedSocket, const char* junk, ssize_t n) -> void { // readProcessor
                static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
            },
            [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                static_cast<HTTPContext*>(connectedSocket->getContext())->onReadError(errnum);
            },
            [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                static_cast<HTTPContext*>(connectedSocket->getContext())->onWriteError(errnum);
            })
            ->listen(port, 5, cert, key, password, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace tls
