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

#ifndef TLS_SOCKETCONNECTION_H
#define TLS_SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tcp/SocketConnection.h"
#include "socket/tcp/tls/SocketReader.h"
#include "socket/tcp/tls/SocketWriter.h"

namespace net::socket::tcp::tls {

    template <typename SocketT>
    using SocketConnection = socket::tcp::SocketConnection<tls::SocketReader<SocketT>, tls::SocketWriter<SocketT>>;

} // namespace net::socket::tcp::tls

#endif // TLS_SOCKETCONNECTION_H
