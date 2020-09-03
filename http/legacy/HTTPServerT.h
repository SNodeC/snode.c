#ifndef LEGACY_HTTPSERVERT_H
#define LEGACY_HTTPSERVERT_H

#include "../HTTPServerT.h"
#include "socket/legacy/SocketServer.h"

namespace http {

    namespace legacy {

        class HTTPServerT : public http::HTTPServerT<net::socket::legacy::SocketServer> {
        public:
            using http::HTTPServerT<net::socket::legacy::SocketServer>::HTTPServerT;
        };

    } // namespace legacy

} // namespace http

#endif // LEGACY_HTTPSERVERT_H
