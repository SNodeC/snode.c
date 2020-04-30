#include "HTTPServer.h"

#include "ConnectedSocket.h"
#include "HTTPContext.h"
#include "ServerSocket.h"
#include "Multiplexer.h"

#include <iostream>

HTTPServer::HTTPServer() : rootDir(".") {}


HTTPServer& HTTPServer::instance() {
    return *new HTTPServer();
}


HTTPServer::~HTTPServer() {
    std::cout << "Done" << std::endl;
}


void HTTPServer::listen(int port) {
    ServerSocket::instance([this] (ConnectedSocket* connectedSocket) -> void {
                                connectedSocket->setContext(new HTTPContext(this, connectedSocket));
                            },
                            [] (ConnectedSocket* connectedSocket) -> void {
                                delete static_cast<HTTPContext*>(connectedSocket->getContext());
                            },
                            [] (ConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
                                static_cast<HTTPContext*>(connectedSocket->getContext())->parseHttpRequest(junk, n);
                            }
                        ).listen(port, 5, 0);

    Multiplexer::run();
}


void HTTPServer::listen(int port, const std::function<void (int err)>& callback) {
    errno = 0;
    
    ServerSocket::instance([this] (ConnectedSocket* connectedSocket) -> void {
                                connectedSocket->setContext(new HTTPContext(this, connectedSocket));
                            },
                            [] (ConnectedSocket* connectedSocket) -> void {
                                delete static_cast<HTTPContext*>(connectedSocket->getContext());
                            },
                            [] (ConnectedSocket* connectedSocket, const char*  junk, ssize_t n) -> void {
                                static_cast<HTTPContext*>(connectedSocket->getContext())->parseHttpRequest(junk, n);
                            }
                        ).listen(port, 5, callback);
    
    callback(errno);

    Multiplexer::run();
}


void HTTPServer::destroy() {
    delete this;
}


void HTTPServer::process(const Request& request, const Response& response) {
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
