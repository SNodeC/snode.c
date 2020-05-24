#include "WebApp.h"

#include "SocketConnection.h"
#include "SSLSocketConnection.h"
#include "HTTPContext.h"
#include "SocketServer.h"



WebApp::WebApp(const std::string& serverRoot) {
    this->serverRoot(serverRoot);
}


WebApp& WebApp::instance(const std::string& serverRoot) {
    return *new WebApp(serverRoot);
}


WebApp::~WebApp() {}


void WebApp::listen(int port) {
    SocketServerBase<SocketConnection>::instance([this] (SocketConnectionInterface* connectedSocket) -> void {
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
    })->listen(port, 5, 0);

    SocketServerBase<SocketConnection>::run();
}


void WebApp::listen(int port, const std::function<void (int err)>& onError) {
    errno = 0;

    SocketServerBase<SocketConnection>::instance([this] (SocketConnectionInterface* connectedSocket) -> void {
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
    })->listen(port, 5, [&] (int err) -> void {
        onError(err);
        if (!err) {
            SocketServerBase<SocketConnection>::run();
        }
    });
}


void WebApp::sslListen(int port) {
    SocketServerBase<SSLSocketConnection>::instance([this] (SocketConnectionInterface* connectedSocket) -> void {
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
    })->listen(port, 5, 0);

    SocketServerBase<SSLSocketConnection>::run();
}


void WebApp::sslListen(int port, const std::function<void (int err)>& onError) {
    errno = 0;

    SocketServerBase<SSLSocketConnection>::instance([this] (SocketConnectionInterface* connectedSocket) -> void {
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
    })->listen(port, 5, [&] (int err) -> void {
        onError(err);
        if (!err) {
            SocketServerBase<SSLSocketConnection>::run();
        }
    });
}


void WebApp::stop() {
    SocketServerBase<SocketConnection>::stop();
}

void WebApp::destroy() {
    delete this;
}
