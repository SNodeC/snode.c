#include "WebApp.h"

#include "HTTPContext.h"
#include "SocketLegacyServer.h"
#include "SocketSSLServer.h"


WebApp::WebApp(const std::string& serverRoot) {
    this->serverRoot(serverRoot);
}


WebApp::~WebApp() {
}


void WebApp::listen(int port, const std::function<void(int err)>& onError) {
    errno = 0;

    SocketLegacyServer::instance(
        [this](SocketConnection* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [](SocketConnection* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [](SocketConnection* connectedSocket, const char* junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [](int errnum) -> void {
            if (errnum) {
                perror("Read from ConnectedSocket");
            }
        },
        [](int errnum) -> void {
            if (errnum) {
                perror("Write to ConnectedSocket");
            }
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

    SocketSSLServer::instance(
        [this](SocketConnection* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [](SocketConnection* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [](SocketConnection* connectedSocket, const char* junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [](int errnum) -> void {
            if (errnum) {
                perror("Read from SSLConnectedSocket");
            }
        },
        [](int errnum) -> void {
            if (errnum) {
                perror("Write to SSLConnectedSocket");
            }
        })
        ->listen(port, 5, cert, key, password, [&](int err) -> void {
            if (onError) {
                onError(err);
            }
        });
}


void WebApp::start() {
    SocketServer::run();
}


void WebApp::stop() {
    SocketServer::stop();
}
