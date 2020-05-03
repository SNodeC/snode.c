#include "HTTPServer.h"

#include "ConnectedSocket.h"
#include "HTTPContext.h"
#include "ServerSocket.h"

#include <stdio.h>


HTTPServer::HTTPServer() : rootDir(".") {}


HTTPServer& HTTPServer::instance() {
    return *new HTTPServer();
}


HTTPServer::~HTTPServer() {}


void HTTPServer::listen(int port) {
    ServerSocket::instance([this] (ConnectedSocket* connectedSocket) -> void {
                                connectedSocket->setContext(new HTTPContext(this, connectedSocket));
                            },
                            [] (ConnectedSocket* connectedSocket) -> void {
                                delete static_cast<HTTPContext*>(connectedSocket->getContext());
                            },
                            [] (ConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
                                static_cast<HTTPContext*>(connectedSocket->getContext())->parseHttpRequest(junk, n);
                            },
                            [] (int errnum) -> void {
                                perror("ConnectedSocket");
                            },
                            [] (int errnum) -> void {
                                perror("ConnectedSocket");
                            }
                        ).listen(port, 5, 0);

    ServerSocket::run();
}


void HTTPServer::listen(int port, const std::function<void (int err)>& onError) {
    errno = 0;
    
    ServerSocket::instance([this] (ConnectedSocket* connectedSocket) -> void {
                                connectedSocket->setContext(new HTTPContext(this, connectedSocket));
                            },
                            [] (ConnectedSocket* connectedSocket) -> void {
                                delete static_cast<HTTPContext*>(connectedSocket->getContext());
                            },
                            [] (ConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
                                static_cast<HTTPContext*>(connectedSocket->getContext())->parseHttpRequest(junk, n);
                            },
                            [] (int errnum) -> void {
                                if (errnum) {
                                    perror("ConnectedSocket");
                                }
                            },
                            [] (int errnum) -> void {
                                if (errnum) {
                                    perror("ConnectedSocket");
                                }
                            }
                        ).listen(port, 5, [&] (int err) -> void {
                            onError(err);
                            if (!err) {
                                ServerSocket::run();
                            }
                        });
}


void HTTPServer::destroy() {
    ServerSocket::stop();
    delete this;
}


void HTTPServer::process(HTTPContext* httpContext) {
    Request request(httpContext);
    Response response(httpContext);
    
    // if GET-Request
    if (request.isGet()) {
        if (getProcessor) {
            getProcessor(request, response);
        }
    }

    // if POST-Request
    if (request.isPost()) {
        if (postProcessor) {
            postProcessor(request, response);
        }
    }

    // if PUT-Request
    if (request.isPut()) {
        if  (putProcessor) {
            putProcessor(request, response);
        }
    }

    // For all:
    if (allProcessor) {
        allProcessor(request, response);
    }
}
