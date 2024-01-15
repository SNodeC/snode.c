/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef APPS_ECHO_MODEL_ECHOSOCKETCONTEXT_H
#define APPS_ECHO_MODEL_ECHOSOCKETCONTEXT_H

#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"

namespace core::socket::stream {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::echo::model {

    class EchoSocketContext : public core::socket::stream::SocketContext {
    public:
        enum class Role { SERVER, CLIENT };

        explicit EchoSocketContext(core::socket::stream::SocketConnection* socketConnection, Role role);

    private:
        void onConnected() override;
        void onDisconnected() override;

        [[nodiscard]] bool onSignal(int signum) override;

        std::size_t onReceivedFromPeer() override;

        Role role;
    };

    class EchoServerSocketContextFactory : public core::socket::stream::SocketContextFactory {
    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override;
    };

    class EchoClientSocketContextFactory : public core::socket::stream::SocketContextFactory {
    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override;
    };

} // namespace apps::echo::model

#endif // APPS_ECHO_MODEL_ECHOSOCKETCONTEXT_H
