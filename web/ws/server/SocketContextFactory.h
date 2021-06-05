#ifndef SOCKETCONTEXTFACTORY_H
#define SOCKETCONTEXTFACTORY_H

#include "net/socket/stream/SocketContextFactory.h"
#include "web/ws/server/SocketContext.h"

namespace web::ws::server {

    template <typename RequestT, typename ResponseT>
    class SocketContextFactory : public net::socket::stream::SocketContextFactory {
        using Request = RequestT;
        using Response = ResponseT;

    public:
        SocketContextFactory(Request& request, Response& response)
            : request(request)
            , response(response) {
        }

    private:
        net::socket::stream::SocketContext* create(net::socket::stream::SocketConnectionBase* socketConnection) const override {
            return SocketContext::create(socketConnection, request, response);
        }

        Request& request;
        Response& response;
    };

} // namespace web::ws::server

#endif // SOCKETCONTEXTFACTORY_H
