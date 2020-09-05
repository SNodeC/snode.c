#ifndef HTTPSERVERT_H
#define HTTPSERVERT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
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
    class HTTPServer {
    public:
        explicit HTTPServer(const std::function<void(typename SocketServer::SocketConnection*)>& onConnect,
                             const std::function<void(Request& req, Response& res)>& onRequestReady,
                             const std::function<void(typename SocketServer::SocketConnection*)>& onDisconnect,
                             const std::map<std::string, std::any>& options = {{}})
            : onConnect(onConnect)
            , onRequestReady(onRequestReady)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        HTTPServer& operator=(const HTTPServer& webApp) = delete;

    protected:
        SocketServer* socketServer() const {
            return new SocketServer(
                [*this](typename SocketServer::SocketConnection* socketConnection) -> void { // onConnect
                    onConnect(socketConnection);
                    socketConnection->template setProtocol<HTTPServerContext*>(new HTTPServerContext(
                        socketConnection, [*this]([[maybe_unused]] Request& req, [[maybe_unused]] Response& res) -> void {
                            onRequestReady(req, res);
                        }));
                },
                [*this](typename SocketServer::SocketConnection* socketConnection) -> void { // onDisconnect
                    onDisconnect(socketConnection);
                    socketConnection->template getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                        delete protocol;
                    });
                },
                [](typename SocketServer::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                    socketConnection->template getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                        protocol->receiveRequestData(junk, junkSize);
                    });
                },
                [](typename SocketServer::SocketConnection* socketConnection, int errnum) -> void { // onReadError
                    socketConnection->template getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                        protocol->onReadError(errnum);
                    });
                },
                [](typename SocketServer::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
                    socketConnection->template getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                        protocol->onWriteError(errnum);
                    });
                },
                options);
        }

    public:
        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr) const {
            errno = 0;

            socketServer()->listen(port, 5, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
        }

        void listen(const std::string host, in_port_t port, const std::function<void(int err)>& onError = nullptr) const {
            errno = 0;

            socketServer()->listen(host, port, 5, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
        }

    protected:
        std::function<void(typename SocketServer::SocketConnection*)> onConnect;
        std::function<void(Request& req, Response& res)> onRequestReady;
        std::function<void(typename SocketServer::SocketConnection*)> onDisconnect;

        std::map<std::string, std::any> options;
    };

} // namespace http

#endif // HTTPSERVERT_H
