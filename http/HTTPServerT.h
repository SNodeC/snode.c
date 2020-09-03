#ifndef HTTPSERVERT_H
#define HTTPSERVERT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "HTTPServerContext.h"

namespace http {

    class Request;
    class Response;

    template <typename SocketServer>
    class HTTPServerT {
    public:
        explicit HTTPServerT(const std::function<void(typename SocketServer::SocketConnectionType*)>& onConnect,
                             const std::function<void(Request& req, Response& res)>& onRequestReady,
                             const std::function<void(Request& req, Response& res)>& onResponseCompleted,
                             const std::function<void(typename SocketServer::SocketConnectionType*)>& onDisconnect)
            : onConnect(onConnect)
            , onRequestReady(onRequestReady)
            , onResponseCompleted(onResponseCompleted)
            , onDisconnect(onDisconnect) {
        }

        HTTPServerT& operator=(const HTTPServerT& webApp) = delete;

        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr) {
            errno = 0;

            (new SocketServer(
                 [*this](typename SocketServer::SocketConnectionType* socketConnection) -> void { // onConnect
                     onConnect(socketConnection);
                     socketConnection->template setProtocol<HTTPServerContext*>(new HTTPServerContext(
                         socketConnection,
                         [*this]([[maybe_unused]] Request& req, [[maybe_unused]] Response& res) -> void {
                             onRequestReady(req, res);
                         },
                         [*this]([[maybe_unused]] Request& req, [[maybe_unused]] Response& res) -> void {
                             onResponseCompleted(req, res);
                         }));
                 },
                 [*this](typename SocketServer::SocketConnectionType* socketConnection) -> void { // onDisconnect
                     onDisconnect(socketConnection);
                     socketConnection->template getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                         delete protocol;
                     });
                 },
                 [](typename SocketServer::SocketConnectionType* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                     socketConnection->template getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                         protocol->receiveRequestData(junk, junkSize);
                     });
                 },
                 [](typename SocketServer::SocketConnectionType* socketConnection, int errnum) -> void { // onReadError
                     socketConnection->template getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                         protocol->onReadError(errnum);
                     });
                 },
                 [](typename SocketServer::SocketConnectionType* socketConnection, int errnum) -> void { // onWriteError
                     socketConnection->template getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                         protocol->onWriteError(errnum);
                     });
                 }))
                ->listen(port, 5, [&](int err) -> void {
                    if (onError) {
                        onError(err);
                    }
                });
        }

    protected:
        std::function<void(typename SocketServer::SocketConnectionType*)> onConnect;
        std::function<void(Request& req, Response& res)> onRequestReady;
        std::function<void(Request& req, Response& res)> onResponseCompleted;
        std::function<void(typename SocketServer::SocketConnectionType*)> onDisconnect;
    };

} // namespace http

#endif // HTTPSERVERT_H
