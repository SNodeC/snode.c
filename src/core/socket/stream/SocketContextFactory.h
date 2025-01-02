/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef CORE_SOCKET_SOCKETCONTEXTFACTORY_H
#define CORE_SOCKET_SOCKETCONTEXTFACTORY_H

namespace core::socket::stream {
    class SocketConnection;
    class SocketContext;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketContextFactory {
    protected:
        SocketContextFactory() = default;

        virtual ~SocketContextFactory();

    public:
        SocketContextFactory(SocketContextFactory&) = delete;
        SocketContextFactory(SocketContextFactory&&) = delete;

        SocketContextFactory& operator=(SocketContextFactory&) = delete;
        SocketContextFactory& operator=(SocketContextFactory&&) = delete;

        virtual core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) = 0;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_SOCKETCONTEXTFACTORY_H
