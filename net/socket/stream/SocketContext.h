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

#ifndef NET_SOCKET_STREAM_SOCKETCONTEXT_H
#define NET_SOCKET_STREAM_SOCKETCONTEXT_H

namespace net::socket::stream {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    class SocketConnectionBase;

    class SocketContext {
    protected:
        explicit SocketContext(SocketConnectionBase* socketConnection);
        virtual ~SocketContext() = default;

    public:
        void sendToPeer(const char* junk, std::size_t junkLen);
        void sendToPeer(const std::string& data);

        std::string getLocalAddressAsString() const;
        std::string getRemoteAddressAsString() const;

        void close();

        void switchSocketProtocol(const SocketContextFactory& socketProtocolFactory);

        void setTimeout(int timeout);

    protected:
        SocketConnectionBase* socketConnection;

    private:
        virtual void onReceiveFromPeer(const char* junk, std::size_t junkLen) = 0;
        virtual void onWriteError(int errnum) = 0;
        virtual void onReadError(int errnum) = 0;

        void receiveFromPeer(const char* junk, std::size_t junkLen);

        virtual void onProtocolConnected();
        virtual void onProtocolDisconnected();

        bool markedForDelete = false;

        friend class SocketConnectionBase;

        template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
        friend class SocketConnection;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCONTEXT_H
