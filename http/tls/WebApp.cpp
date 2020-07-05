#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

#include "HTTPContext.h"
#include "socket/tls/SocketServer.h"


namespace tls {

    void WebApp::listen(int port, const std::function<void(int err)>& onError) {
        errno = 0;

        tls::SocketServer::instance(
            [this](tls::SocketConnection* connectedSocket) -> void { // onConnect
                connectedSocket->setAttribute<HTTPContext*>(new HTTPContext(*this, connectedSocket));
            },
            [](tls::SocketConnection* connectedSocket) -> void { // onDisconnect
                connectedSocket->getAttribute<HTTPContext*>([](HTTPContext*& context) -> void {
                    delete context;
                });
            },
            [](tls::SocketConnection* connectedSocket, const char* junk, ssize_t n) -> void { // onRead
                connectedSocket->getAttribute<HTTPContext*>([&junk, &n](HTTPContext*& context) -> void {
                    context->receiveData(junk, n);
                });
            },
            [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                connectedSocket->getAttribute<HTTPContext*>([&errnum](HTTPContext*& context) -> void {
                    context->onReadError(errnum);
                });
            },
            [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                connectedSocket->getAttribute<HTTPContext*>([&errnum](HTTPContext*& context) -> void {
                    context->onReadError(errnum);
                });
            })
            ->listen(port, 5, cert, key, password, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace tls
