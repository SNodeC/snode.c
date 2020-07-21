#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

#include "HTTPServerContext.h"
#include "socket/legacy/SocketServer.h"


namespace legacy {

    void WebApp::listen(int port, const std::function<void(int err)>& onError) {
        errno = 0;

        (new legacy::SocketServer(
             [this](legacy::SocketConnection* connectedSocket) -> void { // onConnect
                 connectedSocket->setProtocol<HTTPServerContext*>(new HTTPServerContext(*this, connectedSocket));
             },
             [](legacy::SocketConnection* connectedSocket) -> void { // onDisconnect
                 connectedSocket->getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                     delete protocol;
                 });
             },
             [](legacy::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
                 connectedSocket->getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                     protocol->receiveData(junk, junkSize);
                 });
             },
             [](legacy::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onReadError(errnum);
                 });
             },
             [](legacy::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onWriteError(errnum);
                 });
             }))
            ->listen(port, 5, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace legacy
