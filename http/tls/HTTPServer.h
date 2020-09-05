#ifndef TLS_HTTPSERVERT_H
#define TLS_HTTPSERVERT_H

#include "../HTTPServer.h"
#include "socket/tls/SocketServer.h"

namespace http {

    namespace tls {

        class HTTPServer : public http::HTTPServer<net::socket::tls::SocketServer> {
        public:
            using http::HTTPServer<net::socket::tls::SocketServer>::HTTPServer;
        };

    } // namespace tls

} // namespace http

#endif // TLS_HTTPSERVERT_H
