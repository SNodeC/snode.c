/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETCONNECTION_H
#define NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/sock_stream/SocketConnection.h"
#include "socket/sock_stream/legacy/SocketReader.h"
#include "socket/sock_stream/legacy/SocketWriter.h"

namespace net::socket::stream::legacy {

    template <typename SocketT>
    class SocketConnection : public socket::stream::SocketConnection<legacy::SocketReader<SocketT>, legacy::SocketWriter<SocketT>> {
    public:
        using Socket = SocketT;
        using SocketConnectionSuper = socket::stream::SocketConnection<legacy::SocketReader<Socket>, legacy::SocketWriter<Socket>>;

        SocketConnection(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                         const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                         const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                         const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                         const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                         const std::function<void(SocketConnection* socketConnection)>& onDisconnect)
            : socket::stream::SocketConnection<legacy::SocketReader<Socket>, legacy::SocketWriter<Socket>>::SocketConnection(
                  [onConstruct](SocketConnectionSuper* socketConnection) -> void {
                      onConstruct(static_cast<SocketConnection*>(socketConnection));
                  },
                  [onDestruct](SocketConnectionSuper* socketConnection) -> void {
                      onDestruct(static_cast<SocketConnection*>(socketConnection));
                  },
                  [onRead](SocketConnectionSuper* socketConnection, const char* junk, ssize_t junkLen) -> void {
                      onRead(static_cast<SocketConnection*>(socketConnection), junk, junkLen);
                  },
                  [onReadError](SocketConnectionSuper* socketConnection, [[maybe_unused]] int errnum) -> void {
                      onReadError(static_cast<SocketConnection*>(socketConnection), errnum);
                  },
                  [onWriteError](SocketConnectionSuper* socketConnection, [[maybe_unused]] int errnum) -> void {
                      onWriteError(static_cast<SocketConnection*>(socketConnection), errnum);
                  },
                  [onDisconnect](SocketConnectionSuper* socketConnection) -> void {
                      onDisconnect(static_cast<SocketConnection*>(socketConnection));
                  }) {
        }
    };

} // namespace net::socket::stream::legacy

#endif // NET_SOCKET_SOCK_STREAM_LEGACY_SOCKETCONNECTION_H

/*
If using inheritance, try to call as (subclass)
std::function<void(net::socket::stream::tls::SocketConnection<net::socket::ipv4::tcp::tls::Socket>*)>&,
std::function<void(net::socket::stream::tls::SocketConnection<net::socket::ipv4::tcp::tls::Socket>*)>&,
std::function<void(net::socket::stream::tls::SocketConnection<net::socket::ipv4::tcp::tls::Socket>*, const char*, long int)>&,
std::function<void(net::socket::stream::tls::SocketConnection<net::socket::ipv4::tcp::tls::Socket>*, int)>&,
std::function<void(net::socket::stream::tls::SocketConnection<net::socket::ipv4::tcp::tls::Socket>*, int)>&,
std::function<void(net::socket::stream::tls::SocketConnection<net::socket::ipv4::tcp::tls::Socket>*)>&)’

to (baseclass)
const std::function<void(net::socket::stream::SocketConnection<SocketReaderT, SocketWriterT>*)>&,
const std::function<void(net::socket::stream::SocketConnection<SocketReaderT, SocketWriterT>*)>&,
const std::function<void(net::socket::stream::SocketConnection<SocketReaderT, SocketWriterT>*, const char*, long int)>&,
const std::function<void(net::socket::stream::SocketConnection<SocketReaderT, SocketWriterT>*, int)>&,
const std::function<void(net::socket::stream::SocketConnection<SocketReaderT, SocketWriterT>*, int)>&,
const std::function<void(net::socket::stream::SocketConnection<SocketReaderT, SocketWriterT>*)>&)
*/
