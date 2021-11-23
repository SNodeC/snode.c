/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

namespace core::socket::stream {
    class SocketContextFactory;
    class SocketConnection;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketContext {
    protected:
        explicit SocketContext(SocketConnection* socketConnection);
        virtual ~SocketContext() = default;

    public:
        void sendToPeer(const char* junk, std::size_t junkLen);
        void sendToPeer(const std::string& data);
        ssize_t readFromPeer(char* junk, std::size_t junklen);
        void close();

        std::string getLocalAddressAsString() const;
        std::string getRemoteAddressAsString() const;

        SocketContext* switchSocketContext(SocketContextFactory* socketContextFactory);

        void setTimeout(int timeout);

        virtual void onReceiveFromPeer() = 0;
        virtual void onWriteError(int errnum) = 0;
        virtual void onReadError(int errnum) = 0;

        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;

    private:
        SocketConnection* socketConnection;

        template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
        friend class SocketConnectionT;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONTEXT_H
