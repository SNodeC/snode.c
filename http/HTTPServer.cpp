#include "HTTPServer.h"

#include <iostream>
#include <sstream>

#include "ConnectedSocket.h"
#include "HTTPContext.h"

HTTPServer::HTTPServer(int port) : rootDir(".") {
    ServerSocket::instance(port, 
        [&] (ConnectedSocket* cs) -> void {
            cs->setContext(new HTTPContext(this, cs));
            std::cout << "OnConnect" << std::endl;
        },
        [&] (ConnectedSocket* cs) -> void {
            delete static_cast<HTTPContext*>(cs->getContext());
            std::cout << "OnDisconnect" << std::endl;
        },
        [&] (ConnectedSocket* cs, std::string line) -> void {
            static_cast<HTTPContext*>(cs->getContext())->httpRequest(line);
            std::cout << "OnInput" << std::endl;
        });
}


void HTTPServer::process(Request& request, Response& response) {
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
