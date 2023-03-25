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

#ifndef CORE_SOCKET_LOGICALSOCKET_H
#define CORE_SOCKET_LOGICALSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    // Inheritance structure concrete on IPv4 TLS
    //
    // core::socket::LogicalSocket                                                      |    Just config (and coming PhysicalServerSocket)
    // core::socket::stream::LogicalSocketClient = core::socket::LogicalSocket          |    PhysicalServerSocket - will be removed soon
    // core::socket::stream::SocketClient = net::in::stream::SocketClient               |    General connect methods
    // core::socket::stream::tls::SocketClient = core::socket::stream::SocketClient     |    Just Encryption
    // net::in::stream::SocketClient = core::socket::stream::LogicalSocketClient        |    IPv4-Specific connect methods
    // net::in::stream::tls::SocketClient = core::socket::stream::tls::SocketClient     |    Alias Template

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

} // namespace core::socket

#endif // CORE_SOCKET_LOGICALSOCKET_H
