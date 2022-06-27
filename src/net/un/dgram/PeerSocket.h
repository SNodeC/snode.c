/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef NET_UN_DGRAM_PEERSOCKET_H
#define NET_UN_DGRAM_PEERSOCKET_H

#include "net/dgram/PeerSocket.h" // IWYU pragma: export
#include "net/un/dgram/Socket.h"  // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::dgram {

    template <typename ConfigT>
    class PeerSocket : public net::dgram::PeerSocket<ConfigT, net::un::dgram::Socket> {
        using Super = net::dgram::PeerSocket<ConfigT, net::un::dgram::Socket>;

    protected:
        explicit PeerSocket(const std::string& name);

    public:
        using Super::connect;

        using Super::Super;

        void connect(const std::string& sunPath, const std::function<void(const SocketAddress&, int)>& onError);
        void connect(const std::string& remoteSunPath,
                     const std::string& localSunPath,
                     const std::function<void(const SocketAddress&, int)>& onError);
    };

} // namespace net::un::dgram

#endif // NET_UN_DGRAM_PEERSOCKET_H
