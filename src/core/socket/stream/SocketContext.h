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

#ifndef CORE_SOCKET_STREAM_SOCKETCONTEXT_H
#define CORE_SOCKET_STREAM_SOCKETCONTEXT_H

#include "core/socket/SocketContext.h" // IWYU pragma: export

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketContext : public core::socket::SocketContext1 {
    private:
        using Super = core::socket::SocketContext1;

    protected:
        explicit SocketContext(core::socket::SocketConnection* socketConnection);

    public:
        virtual ~SocketContext() = default;

        void shutdownRead();
        void shutdownWrite(bool forceClose = false);
        void shutdown(bool forceClose = false);

    private:
        //        virtual void onConnected();
        //        virtual void onDisconnected();

    private:
        virtual void onWriteError(int errnum) override;
        virtual void onReadError(int errnum) override;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONTEXT_H
