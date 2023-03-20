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

#ifndef NET_CONFIGSOCKET_H
#define NET_CONFIGSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    // template <typename SocketContextFactoryT>
    // using SocketServer =
    // core::socket::TRANSPORT::ENCRYPTION::SocketServer<
    //                                                   net::NET::TRANSPORT::SocketServer<
    //                                                                                     net::NET::TRANSPORT::ENCRYPTION::config::ConfigSocketServer
    //                                                                                    >,
    //                                                   SocketContextFactoryT
    //                                                  >;
    //
    // template <typename ConfigT>
    // class net::NET::TRANSPORT::SocketServer :
    // net::TRANSPORT::SocketServer<net::NET::TRANSPORT::PhysicalServerSocket, ConfigT>
    //
    // template <typename PhysicalServerSocketT, typename ConfigT>
    // class net::TRANSPORT::SocketServer : net::LogicalSocket<ConfigT>
    //
    // net::NET::TRANSPORT::PhysicalServerSocket : net::TRANSPORT::PhysicalServerSocket<net::NET::STREAM::PhysicalSocket>
    //
    // net::TRANSPORT::PhysicalServerSocket<net::NET::TRANSPORT::PhysicalSocket>

    // net::NET::TRANSPORT::PhysicalSocket : net::NET::PhysicalSocket
    //
    // net::NET::PhysicalSocket : net::PhysicalSocket<net::NET::SocketAddress>
    //
    // Inheritance structure
    //
    // net::NET::TRANSPORT::ENCRYPTION::SocketServer
    // core::socket::TRANSPORT::ENCRYPTION::SocketServer
    // core::socket::TRANSPORT::SocketServer
    // net::NET::TRANSPORT::SocketServer
    // net::TRANSPORT::SocketServer
    // net::LogicalSocket
    //
    // net::NET::TRANSPORT::ENCRYPTION::SocketClient
    // core::socket::TRANSPORT::ENCRYPTION::SocketClient
    // core::socket::TRANSPORT::SocketClient
    // net::NET::TRANSPORT::SocketClient
    // net::TRANSPORT::SocketClient
    // net::LogicalSocket
    //
    // net::NET::TRANSPORT::PhysicalServerSocket
    // net::TRANSPORT::PhysicalServerSocket
    // net::NET::TRANSPORT::PhysicalSocket
    // net::NET::PhysicalSocket
    // net::PhysicalSocket
    //
    // net::NET::TRANSPORT::PhysicalClientSocket
    // net::TRANSPORT::PhysicalClientSocket   -> system::connect
    // net::NET::TRANSPORT::PhysicalSocket
    // net::NET::PhysicalSocket
    // net::PhysicalSocket

    template <typename ConfigT>
    class LogicalSocket {
    public:
        using Config = ConfigT;

        explicit LogicalSocket(const std::string& name);

        LogicalSocket(const LogicalSocket&) = default;
        LogicalSocket(LogicalSocket&&) = default;

        LogicalSocket& operator=(const LogicalSocket&) = default;
        LogicalSocket& operator=(LogicalSocket&&) = default;

        virtual ~LogicalSocket();

        Config& getConfig() const;

    protected:
        std::shared_ptr<Config> config;
    };

} // namespace net

#endif // NET_CONFIGSOCKET_H
