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

#ifndef CORE_SOCKET_SOCKETCONTEXT_H
#define CORE_SOCKET_SOCKETCONTEXT_H

namespace utils {
    class Timeval;
} // namespace utils

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    class SocketContext {
    protected:
        SocketContext() = default;

    public:
        SocketContext(const SocketContext&) = delete;
        SocketContext(SocketContext&&) = delete;

        SocketContext& operator=(const SocketContext&) = delete;
        SocketContext& operator=(SocketContext&&) = delete;

    protected:
        virtual ~SocketContext();

    public:
        virtual void setTimeout(const utils::Timeval& timeout) = 0;

        void sendToPeer(const std::string& data) const;
        virtual void sendToPeer(const char* chunk, std::size_t chunkLen) const = 0;

        virtual std::size_t readFromPeer(char* chunk, std::size_t chunklen) const = 0;

        virtual void close() = 0;

    protected:
        virtual std::size_t onReceivedFromPeer() = 0;

        virtual bool onSignal(int sig) = 0;

        virtual void onWriteError(int errnum) = 0;
        virtual void onReadError(int errnum) = 0;
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKETCONTEXT_H
