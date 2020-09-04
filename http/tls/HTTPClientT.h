#ifndef TLS_HTTPCLIENTT_H
#define TLS_HTTPCLIENTT_H

#include "../HTTPClientT.h"
#include "socket/tls/SocketClient.h"

namespace http::tls {

    class HTTPClientT : public http::HTTPClientT<net::socket::tls::SocketClient> {
    public:
        using http::HTTPClientT<net::socket::tls::SocketClient>::HTTPClientT;
    };

} // namespace http::tls

#endif // TLS_HTTPCLIENTT_H
