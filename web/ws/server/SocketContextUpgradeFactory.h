#ifndef SOCKETCONTEXTFACTORY_H
#define SOCKETCONTEXTFACTORY_H

#include "web/http/server/SocketContextUpgradeFactory.h"
#include "web/http/server/SocketContextUpgradeInterface.h"
#include "web/ws/server/SocketContext.h"
#include "web/ws/server/SubProtocolSelector.h"

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

namespace web::ws::server {

    class SocketContextUpgradeFactory : public web::http::server::SocketContextUpgradeFactory {
    public:
        SocketContextUpgradeFactory();
        ~SocketContextUpgradeFactory();

        std::string name() override;
        std::string type() override;

    private:
        web::ws::server::SocketContext* create(net::socket::stream::SocketConnectionBase* socketConnection) const override;
    };

    class SocketContextUpgradeInterface : public web::http::server::SocketContextUpgradeInterface {
    public:
        web::http::server::SocketContextUpgradeFactory* create() override;
    };

} // namespace web::ws::server

#endif // SOCKETCONTEXTFACTORY_H
