#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPServer.h"

#include "HTTPServerContext.h"
#include "Request.h"
#include "Response.h"
#include "socket/legacy/SocketConnection.h"
#include "socket/legacy/SocketServer.h"

namespace legacy {

    HTTPServer::HTTPServer(const std::function<void(Request& req, Response& res)>& onRequest)
        : onRequest(onRequest) {
    }

    void HTTPServer::listen(in_port_t port, const std::function<void(int err)>& onError) {
        errno = 0;

        (new legacy::SocketServer(
             [this](legacy::SocketConnection* connectedSocket) -> void { // onConnect
                 connectedSocket->setProtocol<HTTPServerContext*>(
                     new HTTPServerContext(connectedSocket, [this](Request& req, Response& res) -> void {
                         onRequest(req, res);
                     }));
                 ;
             },
             [](legacy::SocketConnection* connectedSocket) -> void { // onDisconnect
                 connectedSocket->getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                     delete protocol;
                 });
             },
             [](legacy::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
                 connectedSocket->getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                     protocol->receiveRequestData(junk, junkSize);
                 });
             },
             [](legacy::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onReadError(errnum);
                 });
             },
             [](legacy::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onWriteError(errnum);
                 });
             }))
            ->listen(port, 5, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace legacy
