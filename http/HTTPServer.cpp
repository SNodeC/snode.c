#include "HTTPServer.h"

#include "ConnectedSocket.h"
#include "HTTPContext.h"


HTTPServer::HTTPServer(int port)
: rootDir(".") {
    ServerSocket::instance(port,
        [&] (ConnectedSocket* connectedSocket) -> void {
            connectedSocket->setContext(new HTTPContext(this, connectedSocket));
        },
        [&] (ConnectedSocket* connectedSocket) -> void {
            delete static_cast<HTTPContext*>(connectedSocket->getContext());
        },
        [&] (ConnectedSocket* connectedSocket, std::string line) -> void {
            static_cast<HTTPContext*>(connectedSocket->getContext())->parseHttpRequest(line);
        }
    );
}


HTTPServer& HTTPServer::instance(int port) {
    return *new HTTPServer(port);
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
