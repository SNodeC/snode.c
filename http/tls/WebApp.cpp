#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

#include "HTTPContext.h"
#include "socket/tls/SocketServer.h"


namespace tls {

    void WebApp::listen(int port, const std::function<void(int err)>& onError) {
        errno = 0;

        (new tls::SocketServer(
             [this](tls::SocketConnection* connectedSocket) -> void { // onConnect
                 connectedSocket->setProtocol<HTTPContext*>(new HTTPContext(*this, connectedSocket));
             },
             [](tls::SocketConnection* connectedSocket) -> void { // onDisconnect
                 connectedSocket->getProtocol<HTTPContext*>([](HTTPContext*& context) -> void {
                     delete context;
                 });
             },
             [](tls::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
                 connectedSocket->getProtocol<HTTPContext*>([&junk, &junkSize](HTTPContext*& context) -> void {
                     context->receiveData(junk, junkSize);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                 connectedSocket->getProtocol<HTTPContext*>([&errnum](HTTPContext*& context) -> void {
                     context->onReadError(errnum);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                 connectedSocket->getProtocol<HTTPContext*>([&errnum](HTTPContext*& context) -> void {
                     context->onReadError(errnum);
                 });
             }))
            ->listen(port, 5, cert, key, password, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace tls
