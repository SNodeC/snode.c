#ifndef NET_SOCKET_IPV4_TCP_LEGACY_SOCKETSERVER_H
#define NET_SOCKET_IPV4_TCP_LEGACY_SOCKETSERVER_H

#include "Socket.h"
#include "socket/tcp/legacy/SocketListener.h"
#include "socket/tcp/legacy/SocketServer.h"

namespace net::socket::ipv4::tcp::legacy {

    using SocketServer = net::socket::tcp::legacy::SocketServer<net::socket::ipv4::tcp::legacy::Socket>;

} // namespace net::socket::ipv4::tcp::legacy

#endif // NET_SOCKET_IPV4_TCP_LEGACY_SOCKETSERVER_H
