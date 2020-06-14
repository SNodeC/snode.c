#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

#include "HTTPContext.h"
#include "socket/legacy/SocketServer.h"
#include "socket/tls/SocketServer.h"


WebApp::WebApp(const std::string& rootDir) {
    this->serverRoot(rootDir);
}


void WebApp::listen(int port, const std::function<void(int err)>& onError) {
    errno = 0;

    legacy::SocketServer::instance(
        [this](SocketConnection* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [](SocketConnection* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [](SocketConnection* connectedSocket, const char* junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [](SocketConnection* connectedSocket, int errnum) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->onReadError(errnum);
        },
        [](SocketConnection* connectedSocket, int errnum) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->onWriteError(errnum);
        })
        ->listen(port, 5, [&](int err) -> void {
            if (onError) {
                onError(err);
            }
        });
}


void WebApp::sslListen(int port, const std::string& cert, const std::string& key, const std::string& password,
                       const std::function<void(int err)>& onError) {
    errno = 0;

    tls::SocketServer::instance(
        [this](SocketConnection* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [](SocketConnection* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [](SocketConnection* connectedSocket, const char* junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [](SocketConnection* connectedSocket, int errnum) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->onReadError(errnum);
        },
        [](SocketConnection* connectedSocket, int errnum) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->onWriteError(errnum);
        })
        ->listen(port, 5, cert, key, password, [&](int err) -> void {
            if (onError) {
                onError(err);
            }
        });
}


void WebApp::start(int argc, char** argv) {
    SocketServer::start(argc, argv);
}


void WebApp::stop() {
    SocketServer::stop();
}
