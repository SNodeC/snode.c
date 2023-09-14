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

#ifndef CORE_SOCKET_SOCKETCONTEXT_H
#define CORE_SOCKET_SOCKETCONTEXT_H

namespace utils {
    class Timeval;
} // namespace utils

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // IWYU pragma: export
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

        virtual ~SocketContext() = default;

        virtual void setTimeout(const utils::Timeval& timeout) = 0;

        virtual void sendToPeer(const char* junk, std::size_t junkLen) const = 0;
        void sendToPeer(const std::string& data) const;
        virtual std::size_t readFromPeer(char* junk, std::size_t junklen) const = 0;

        virtual void close() = 0;

    protected:
        virtual std::size_t onReceivedFromPeer() = 0;

        virtual void onExit(int sig);

        virtual void onWriteError(int errnum);
        virtual void onReadError(int errnum);
    };

} // namespace core::socket

#endif // CORE_SOCKET_SOCKETCONTEXT_H
