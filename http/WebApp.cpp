#include "WebApp.h"

#include "SocketLegacyConnection.h"
#include "SocketSSLConnection.h"
#include "HTTPContext.h"
#include "SocketServerBase.h"


WebApp::WebApp(const std::string& serverRoot) {
    this->serverRoot(serverRoot);
}


WebApp& WebApp::instance(const std::string& serverRoot) {
    return *new WebApp(serverRoot);
}


WebApp::~WebApp() {}


void WebApp::listen(int port) {
    SocketLegacyServer::instance(
        [this] (SocketConnectionInterface* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [] (SocketConnectionInterface* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [] (SocketConnectionInterface* connectedSocket, const char*  junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [] (int errnum) -> void {
            perror("Read from ConnectedSocket");
        },
        [] (int errnum) -> void {
            perror("Write to ConnectedSocket");
        }
    )->listen(port, 5, 0);

    SocketServer::run();
}


void WebApp::listen(int port, const std::function<void (int err)>& onError) {
    errno = 0;

    SocketLegacyServer::instance(
        [this] (SocketConnectionInterface* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [] (SocketConnectionInterface* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [] (SocketConnectionInterface* connectedSocket, const char*  junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [] (int errnum) -> void {
            if (errnum) {
                perror("Read from ConnectedSocket");
            }
        },
        [] (int errnum) -> void {
            if (errnum) {
                perror("Write to ConnectedSocket");
            }
        }
    )->listen(port, 5,
        [&] (int err) -> void {
            onError(err);
            if (!err) {
                SocketServer::run();
            }
        }
    );
}


void WebApp::sslListen(int port, const std::string& cert, const std::string& key, const std::string& password) {
    SocketSSLServer::instance(
        [this] (SocketConnectionInterface* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [] (SocketConnectionInterface* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [] (SocketConnectionInterface* connectedSocket, const char*  junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [] (int errnum) -> void {
            perror("Read from SSLConnectedSocket");
        },
        [] (int errnum) -> void {
            perror("Write to SSLConnectedSocket");
        }
    )->listen(port, 5, cert, key, password, 0);

    SocketServer::run();
}


void WebApp::sslListen(int port, const std::string& cert, const std::string& key, const std::string& password, const std::function<void (int err)>& onError) {
    errno = 0;

    SocketSSLServer::instance(
        [this] (SocketConnectionInterface* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [] (SocketConnectionInterface* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [] (SocketConnectionInterface* connectedSocket, const char*  junk, ssize_t n) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
        },
        [] (int errnum) -> void {
            if (errnum) {
                perror("Read from SSLConnectedSocket");
            }
        },
        [] (int errnum) -> void {
            if (errnum) {
                perror("Write to SSLConnectedSocket");
            }
        }
    )->listen(port, 5, cert, key, password,
        [&] (int err) -> void {
            onError(err);
            if (!err) {
                SocketServer::run();
            }
        }
    );
}


void WebApp::stop() {
    SocketServer::stop();
}


void WebApp::destroy() {
    delete this;
}
