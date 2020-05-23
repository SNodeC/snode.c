#include "WebApp.h"

#include "ConnectedSocket.h"
#include "SSLConnectedSocket.h"
#include "HTTPContext.h"
#include "ServerSocket.h"



WebApp::WebApp(const std::string& serverRoot) {
    this->serverRoot(serverRoot);
}


WebApp& WebApp::instance(const std::string& serverRoot) {
    return *new WebApp(serverRoot);
}


WebApp::~WebApp() {}


void WebApp::listen(int port) {
    BaseServerSocket<ConnectedSocket>::instance([this] (ConnectedSocket* connectedSocket) -> void {
                                connectedSocket->setContext(new HTTPContext(this, connectedSocket));
                           },
                           [] (ConnectedSocket* connectedSocket) -> void {
                                delete static_cast<HTTPContext*>(connectedSocket->getContext());
                           },
                           [] (ConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
                                static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
                           },
                           [] (int errnum) -> void {
                                perror("ConnectedSocket");
                           },
                           [] (int errnum) -> void {
                                perror("ConnectedSocket");
                           }
                        )->listen(port, 5, 0);

    BaseServerSocket<ConnectedSocket>::run();
}


void WebApp::listen(int port, const std::function<void (int err)>& onError) {
    errno = 0;
    
    BaseServerSocket<ConnectedSocket>::instance([this] (ConnectedSocket* connectedSocket) -> void {
                                connectedSocket->setContext(new HTTPContext(this, connectedSocket));
                            },
                            [] (ConnectedSocket* connectedSocket) -> void {
                                delete static_cast<HTTPContext*>(connectedSocket->getContext());
                            },
                            [] (ConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
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
                        )->listen(port, 5, [&] (int err) -> void {
                            onError(err);
                            if (!err) {
                                BaseServerSocket<ConnectedSocket>::run();
                            }
                        });
}


void WebApp::sslListen(int port) {
    BaseServerSocket<SSLConnectedSocket>::instance([this] (SSLConnectedSocket* connectedSocket) -> void {
        connectedSocket->setContext(new HTTPContext(this, connectedSocket));
    },
    [] (SSLConnectedSocket* connectedSocket) -> void {
        delete static_cast<HTTPContext*>(connectedSocket->getContext());
    },
    [] (SSLConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
        static_cast<HTTPContext*>(connectedSocket->getContext())->receiveRequest(junk, n);
    },
    [] (int errnum) -> void {
        perror("ConnectedSocket");
    },
    [] (int errnum) -> void {
        perror("ConnectedSocket");
    }
    )->listen(port, 5, 0);
    
    BaseServerSocket<SSLConnectedSocket>::run();
}


void WebApp::sslListen(int port, const std::function<void (int err)>& onError) {
    errno = 0;
    
    BaseServerSocket<SSLConnectedSocket>::instance([this] (SSLConnectedSocket* connectedSocket) -> void {
        connectedSocket->setContext(new HTTPContext(this, connectedSocket));
    },
    [] (SSLConnectedSocket* connectedSocket) -> void {
        delete static_cast<HTTPContext*>(connectedSocket->getContext());
    },
    [] (SSLConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
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
    )->listen(port, 5, [&] (int err) -> void {
        onError(err);
        if (!err) {
            BaseServerSocket<SSLConnectedSocket>::run();
        }
    });
}


void WebApp::stop() {
    BaseServerSocket<ConnectedSocket>::stop();
}

void WebApp::destroy() {
    delete this;
}
