#ifndef HTTPSERVERCONTEXTFACTORY_H
#define HTTPSERVERCONTEXTFACTORY_H

#include "http/server/http/HTTPServerContext.h"
#include "net/socket/stream/SocketConnectionBase.h"
#include "net/socket/stream/SocketProtocolFactory.h"

namespace http::server {

    template <typename RequestT, typename ResponseT>
    class HTTPServerContextFactory : public net::socket::stream::SocketProtocolFactory {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        HTTPServerContextFactory(const std::function<void(Request& req, Response& res)>& onRequestReady)
            : onRequestReady(onRequestReady) {
        }

        net::socket::stream::SocketProtocol* create(net::socket::stream::SocketConnectionBase* socketConnection) const override {
            return new HTTPServerContext<Request, Response>(socketConnection, onRequestReady);
        }

    protected:
        std::function<void(Request& req, Response& res)> onRequestReady;
    };

} // namespace http::server

#endif // HTTPSERVERCONTEXTFACTORY_H
