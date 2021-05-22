#ifndef HTTP_SERVER_SERVERT_H
#define HTTP_SERVER_SERVERT_H

#include "web/server/http/HTTPServerContext.hpp"
#include "web/server/http/HTTPServerContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::server {

    template <template <typename SocketProtocolT> typename SocketServerT, typename RequestT, typename ResponseT>
    class Server {
    public:
        using Request = RequestT;
        using Response = ResponseT;
        using SocketServer = SocketServerT<web::server::http::HTTPServerContextFactory<Request, Response>>; // this makes it an HTTP server
        using SocketConnection = typename SocketServer::SocketConnection;
        using SocketAddress = typename SocketConnection::SocketAddress;

        Server(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request& req, Response& res)>& onRequestReady,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : socketServer(
                  [onConnect](const SocketAddress& localAddress,
                              const SocketAddress& remoteAddress) -> void { // OnConnect
                      onConnect(localAddress, remoteAddress);
                  },
                  [onConnected, onRequestReady](SocketConnection* socketConnection) -> void { // onConnected.
                      onConnected(socketConnection);
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      onDisconnect(socketConnection);
                  },
                  options) {
            socketServer.getSocketProtocol()->setOnRequestReady(onRequestReady); //.setOnRequestReady(onRequestReady);
        }

        void listen(uint16_t port, const std::function<void(int err)>& onError) const {
            socketServer.listen(SocketAddress(port), 5, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int err)>& onError) const {
            socketServer.listen(SocketAddress(ipOrHostname, port), 5, onError);
        }

        void onConnect(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect) {
            socketServer.onConnect(onConnect);
        }

        void onConnected(const std::function<void(SocketConnection*)>& onConnected) {
            socketServer.onConnected(onConnected);
        }

        void onDisconnect(const std::function<void(SocketConnection*)>& onDisconnect) {
            socketServer.onDisconnect(onDisconnect);
        }

    protected:
        SocketServer socketServer;

        std::function<void(Request& req, Response& res)> onRequestReady;
    };

} // namespace web::server

#endif // HTTP_SERVER_SERVERT_H
