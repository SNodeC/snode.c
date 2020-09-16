#ifndef SERVERT_H
#define SERVERT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <easylogging++.h>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "ServerContext.h"

namespace http {

    class Request;
    class Response;

    template <typename SocketServerT>
    class Server {
    public:
        using SocketServer = SocketServerT;
        using SocketConnection = typename SocketServer::SocketConnection;

        explicit Server(const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(Request& req, Response& res)>& onRequestReady,
                        const std::function<void(Request& req, Response& res)>& onRequestCompleted,
                        const std::function<void(SocketConnection*)>& onDisconnect, const std::map<std::string, std::any>& options = {{}})
            : onConnect(onConnect)
            , onRequestReady(onRequestReady)
            , onRequestCompleted(onRequestCompleted)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        Server& operator=(const Server& webApp) = delete;

    protected:
        SocketServer* socketServer() const {
            return new SocketServer(
                [*this](SocketConnection* socketConnection) -> void { // onConnect
                    onConnect(socketConnection);
                    socketConnection->template setContext<ServerContext*>(new ServerContext(
                        socketConnection,
                        [*this](Request& req, Response& res) -> void {
                            onRequestReady(req, res);
                        },
                        [*this](Request& req, Response& res) -> void {
                            onRequestCompleted(req, res);
                        }));
                },
                [*this](SocketConnection* socketConnection) -> void { // onDisconnect
                    onDisconnect(socketConnection);
                    socketConnection->template getContext<ServerContext*>([](ServerContext*& protocol) -> void {
                        delete protocol;
                    });
                },
                [](SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                    socketConnection->template getContext<ServerContext*>([&junk, &junkSize](ServerContext*& protocol) -> void {
                        protocol->receiveRequestData(junk, junkSize);
                    });
                },
                [](SocketConnection* socketConnection, int errnum) -> void { // onReadError
                    socketConnection->template getContext<ServerContext*>([&errnum](ServerContext*& protocol) -> void {
                        protocol->onReadError(errnum);
                    });
                },
                [](SocketConnection* socketConnection, int errnum) -> void { // onWriteError
                    socketConnection->template getContext<ServerContext*>([&errnum](ServerContext*& protocol) -> void {
                        protocol->onWriteError(errnum);
                    });
                },
                options);
        }

    public:
        void listen(in_port_t port, const std::function<void(int err)>& onError) const {
            errno = 0;

            socketServer()->listen(port, 5, onError);
        }

        void listen(const std::string host, in_port_t port, const std::function<void(int err)>& onError) const {
            errno = 0;

            socketServer()->listen(host, port, 5, onError);
        }

    protected:
        std::function<void(SocketConnection*)> onConnect;
        std::function<void(Request& req, Response& res)> onRequestReady;
        std::function<void(Request& req, Response& res)> onRequestCompleted;
        std::function<void(SocketConnection*)> onDisconnect;

        std::map<std::string, std::any> options;
    };

} // namespace http

#endif // SERVERT_H
