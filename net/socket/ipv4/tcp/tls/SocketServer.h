#ifndef NET_SOCKET_IPV4_TCP_TLS_SOCKETSERVER_H
#define NET_SOCKET_IPV4_TCP_TLS_SOCKETSERVER_H

#include "Socket.h"
#include "socket/tcp/tls/SocketServer.h"

namespace net::socket::ipv4::tcp::tls {

    using SocketServer = net::socket::tcp::tls::SocketServer<net::socket::ipv4::tcp::tls::Socket>;

}

#endif // NET_SOCKET_IPV4_TCP_TLS_SOCKETSERVER_H
