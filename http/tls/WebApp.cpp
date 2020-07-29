#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

#include "HTTPServerContext.h"
#include "socket/tls/SocketServer.h"

namespace tls {

    void WebApp::listen(int port, const std::function<void(int err)>& onError) {
        errno = 0;

        (new tls::SocketServer(
             [this](tls::SocketConnection* connectedSocket) -> void { // onConnect
                 connectedSocket->setProtocol<HTTPServerContext*>(new HTTPServerContext(*this, connectedSocket));
             },
             [](tls::SocketConnection* connectedSocket) -> void { // onDisconnect
                 connectedSocket->getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                     delete protocol;
                 });
             },
             [](tls::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
                 connectedSocket->getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                     protocol->receiveRequestData(junk, junkSize);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onReadError(errnum);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onWriteError(errnum);
                 });
             }))
            ->listen(port, 5, cert, key, password, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace tls
