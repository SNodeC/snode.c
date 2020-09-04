#ifndef LEGACY_HTTPCLIENTT_H
#define LEGACY_HTTPCLIENTT_H

#include "../HTTPClientT.h"
#include "socket/legacy/SocketClient.h"

namespace http {

    namespace legacy {

        class HTTPClientT : public http::HTTPClientT<net::socket::legacy::SocketClient> {
        public:
            using http::HTTPClientT<net::socket::legacy::SocketClient>::HTTPClientT;
        };

    } // namespace legacy

} // namespace http

#endif // LEGACY_HTTPCLIENTT_H
