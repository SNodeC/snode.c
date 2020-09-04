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
        explicit HTTPServerT(const std::function<void(typename SocketServer::SocketConnection*)>& onConnect,
                             const std::function<void(Request& req, Response& res)>& onRequestReady,
                             const std::function<void(Request& req, Response& res)>& onResponseCompleted,
                             const std::function<void(typename SocketServer::SocketConnection*)>& onDisconnect)
            : onConnect(onConnect)
            , onRequestReady(onRequestReady)
            , onResponseCompleted(onResponseCompleted)
            , onDisconnect(onDisconnect) {
        }

        HTTPServerT& operator=(const HTTPServerT& webApp) = delete;

    protected:
        SocketServer* socketServer() {
            return new SocketServer(
                [*this](typename SocketServer::SocketConnection* socketConnection) -> void { // onConnect
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
                });
        }

    public:
        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr) {
            errno = 0;

            socketServer()->listen(port, 5, [&](int err) -> void {
                if (onError) {
                    onError(err);
                }
            });
        }

        void listen(const std::string host, in_port_t port, const std::function<void(int err)>& onError = nullptr) {
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
        std::function<void(Request& req, Response& res)> onResponseCompleted;
        std::function<void(typename SocketServer::SocketConnection*)> onDisconnect;
    };

} // namespace http

#endif // HTTPSERVERT_H
