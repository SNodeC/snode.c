#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPServer.h"

#include "HTTPServerContext.h"
#include "Request.h"
#include "Response.h"
#include "socket/tls/SocketConnection.h"
#include "socket/tls/SocketServer.h"

namespace tls {

    HTTPServer::HTTPServer(const std::string& cert, const std::string& key, const std::string& password,
                           const std::function<void(Request& req, Response& res)>& onRequest)
        : onRequest(onRequest)
        , cert(cert)
        , key(key)
        , password(password) {
    }

    void HTTPServer::listen(in_port_t port, const std::function<void(int err)>& onError) {
        errno = 0;

        (new tls::SocketServer(
             [this](tls::SocketConnection* connectedSocket) -> void { // onConnect
                 connectedSocket->setProtocol<HTTPServerContext*>(
                     new HTTPServerContext(connectedSocket, [this](Request& req, Response& res) -> void {
                         onRequest(req, res);
                     }));
                 ;
             },
             [](tls::SocketConnection* connectedSocket) -> void { // onDisconnect
                 connectedSocket->getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                     delete protocol;
                 });
             },
             [](tls::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
                 connectedSocket->getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                     protocol->receiveRequestData(junk, junkSize);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onReadError(errnum);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onWriteError(errnum);
                 });
             },
             cert, key, password))
            ->listen(port, 5, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
    }

} // namespace tls
