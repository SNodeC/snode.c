#include "WebApp.h"

#include "ConnectedSocket.h"
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
    ServerSocket::instance([this] (ConnectedSocket* connectedSocket) -> void {
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

    ServerSocket::run();
}


void WebApp::listen(int port, const std::function<void (int err)>& onError) {
    errno = 0;
    
    ServerSocket::instance([this] (ConnectedSocket* connectedSocket) -> void {
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
                                ServerSocket::run();
                            }
                        });
}

void WebApp::stop() {
    ServerSocket::stop();
}

void WebApp::destroy() {
    delete this;
}
