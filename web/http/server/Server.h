#ifndef WEB_HTTP_SERVER_SERVERT_H
#define WEB_HTTP_SERVER_SERVERT_H

#include "web/http/server/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::server {

    template <template <typename SocketContextFactoryT> typename SocketServerT, typename RequestT, typename ResponseT>
    class Server {
    public:
        using Request = RequestT;
        using Response = ResponseT;
        using SocketServer = SocketServerT<web::http::server::SocketContextFactory<Request, Response>>; // this makes it an HTTP server
        using SocketConnection = typename SocketServer::SocketConnection;
        using SocketAddress = typename SocketConnection::SocketAddress;

        Server(const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request&, Response&)>& onRequestReady,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : socketServer(
                  [onConnect](const SocketAddress& localAddress,
                              const SocketAddress& remoteAddress) -> void { // OnConnect
                      onConnect(localAddress, remoteAddress);
                  },
                  [onConnected](SocketConnection* socketConnection) -> void { // onConnected.
                      onConnected(socketConnection);
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      onDisconnect(socketConnection);
                  },
                  options) {
            socketServer.getSocketContextFactory()->setOnRequestReady(onRequestReady);
        }

        void listen(uint16_t port, const std::function<void(int)>& onError) const {
            socketServer.listen(SocketAddress(port), 5, onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) const {
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

        std::function<void(Request&, Response&)> onRequestReady;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SERVERT_H
