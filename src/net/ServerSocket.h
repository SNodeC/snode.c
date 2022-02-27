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

#ifndef NET_SERVERSOCKET_H
#define NET_SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename ConfigT, typename SocketT>
    class ServerSocket {
    protected:
        explicit ServerSocket(const std::string& name);

        ServerSocket(const ServerSocket&) = default;

        virtual ~ServerSocket() = default;

    public:
        using Config = ConfigT;
        using Socket = SocketT;
        using SocketAddress = typename Socket::SocketAddress;

        virtual void listen(const std::function<void(const SocketAddress&, int)>& onError) const = 0;

        void listen(const SocketAddress& localAddress, int backlog, const std::function<void(const SocketAddress&, int)>& onError) const;

        Config& getConfig();

    protected:
        std::shared_ptr<Config> config;
    };

} // namespace net

#endif // NET_SERVERSOCKET_H
