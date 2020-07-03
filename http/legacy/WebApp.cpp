#include "WebApp.h"

#include "HTTPContext.h"
#include "socket/legacy/SocketServer.h"


namespace legacy {

    void WebApp::listen(int port, const std::function<void(int err)>& onError) {
        errno = 0;

        legacy::SocketServer::instance(
            [this](legacy::SocketConnection* connectedSocket) -> void {
                connectedSocket->setContext(new HTTPContext(*this, connectedSocket));
            },
            [](legacy::SocketConnection* connectedSocket) -> void {
                delete static_cast<HTTPContext*>(connectedSocket->getContext());
            },
            [](legacy::SocketConnection* connectedSocket, const char* junk, ssize_t n) -> void {
                static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
            },
            [](legacy::SocketConnection* connectedSocket, int errnum) -> void {
                static_cast<HTTPContext*>(connectedSocket->getContext())->onReadError(errnum);
            },
            [](legacy::SocketConnection* connectedSocket, int errnum) -> void {
                static_cast<HTTPContext*>(connectedSocket->getContext())->onWriteError(errnum);
            })
            ->listen(port, 5, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace legacy
