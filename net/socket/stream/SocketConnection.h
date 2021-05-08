/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_STREAM_SOCKETCONNECTION_H
#define NET_SOCKET_STREAM_SOCKETCONNECTION_H

#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
    class SocketConnection
        : public SocketConnectionBase
        , public SocketReaderT
        , public SocketWriterT {
    public:
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using SocketAddress = SocketAddressT;

        SocketConnection() = delete;

    protected:
        SocketConnection(int fd,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(const char* junk, std::size_t junkLen)>& onRead,
                         const std::function<void(int errnum)>& onReadError,
                         const std::function<void(int errnum)>& onWriteError,
                         const std::function<void()>& onDisconnect)
            : SocketReader(onRead, onReadError)
            , SocketWriter(onWriteError)
            , localAddress(localAddress)
            , remoteAddress(remoteAddress)
            , onDisconnect(onDisconnect) {
            this->attach(fd);
        }

        virtual ~SocketConnection() = default;

    private:
        void unobserved() override {
            onDisconnect();
            delete this;
        }

    public:
        void enqueue(const char* junk, std::size_t junkLen) override {
            SocketWriter::enqueue(junk, junkLen);
        }

        void enqueue(const std::string& data) override {
            enqueue(data.data(), data.size());
        }

        void close(bool instantly = false) final {
            SocketReader::disable();
            if (instantly) {
                SocketWriter::disable();
            }
        }

        const SocketAddress& getRemoteAddress() const {
            return remoteAddress;
        }

        void setRemoteAddress(const SocketAddress& remoteAddress) {
            this->remoteAddress = remoteAddress;
        }

        const SocketAddress& getLocalAddress() const {
            return localAddress;
        }

        void setLocalAddress(const SocketAddress& localAddress) {
            this->localAddress = localAddress;
        }

    private:
        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        std::function<void()> onDisconnect;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCONNECTION_H
