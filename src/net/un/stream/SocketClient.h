/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef NET_UN_STREAM_SOCKETCLIENT_H
#define NET_UN_STREAM_SOCKETCLIENT_H

#include "core/socket/stream/LogicalSocketClient.h" // IWYU pragma: export
#include "net/un/stream/PhysicalClientSocket.h"     // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::stream {

    template <typename SocketClientT>
    class SocketClient : public SocketClientT {
    private:
        using Super = SocketClientT;

    protected:
        using Super::Super;

    public:
        using Super::connect;

        void connect(const std::string& sunPath, const std::function<void(const SocketAddress&, int)>& onError) const {
            connect(SocketAddress(sunPath), onError);
        }

        void connect(const std::string& sunPath,
                     const std::string& bindSunPath,
                     const std::function<void(const SocketAddress&, int)>& onError) const {
            connect(SocketAddress(sunPath), SocketAddress(bindSunPath), onError);
        }
    };

} // namespace net::un::stream

#endif // NET_UN_STREAM_SOCKETCLIENT_H
