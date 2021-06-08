#ifndef SOCKETCONTEXTFACTORY_H
#define SOCKETCONTEXTFACTORY_H

#include "web/http/server/SocketContextUpgradeFactory.h"
#include "web/ws/server/SocketContext.h"

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

namespace web::ws::server {

    class SocketContextFactory : public web::http::server::SocketContextUpgradeFactory {
    public:
        SocketContextFactory() = default;

        std::string name() override {
            return "websocket";
        }

        std::string type() override {
            return "server";
        }

    private:
        virtual net::socket::stream::SocketContext* create(net::socket::stream::SocketConnectionBase* socketConnection) const override {
            return SocketContext::create(socketConnection, *request, *response);
        }
    };

} // namespace web::ws::server

#endif // SOCKETCONTEXTFACTORY_H
