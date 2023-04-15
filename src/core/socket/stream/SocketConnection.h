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

#ifndef CORE_SOCKET_STREAM_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_SOCKETCONNECTION_H

#include "core/socket/SocketConnection.h"

namespace core::socket::stream {
    class SocketContextFactory;
    class SocketContext;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#include <cstddef>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketConnection : public core::socket::SocketConnection {
    public:
        SocketConnection() = default;

        ~SocketConnection() override;

        core::socket::stream::SocketContext* switchSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory);

        bool isValid();

    protected:
        core::socket::stream::SocketContext* setSocketContext(core::socket::stream::SocketContextFactory* socketContextFactory);

        core::socket::stream::SocketContext* socketContext = nullptr;
        core::socket::stream::SocketContext* newSocketContext = nullptr;
    };

    template <typename PhysicalSocketT,
              template <typename PhysicalSocket>
              typename SocketReaderT,
              template <typename PhysicalSocket>
              typename SocketWriterT>
    class SocketConnectionT
        : public SocketConnection
        , protected SocketReaderT<PhysicalSocketT>
        , protected SocketWriterT<PhysicalSocketT> {
    protected:
        using Super = core::socket::SocketConnection;

        using PhysicalSocket = PhysicalSocketT;
        using SocketReader = SocketReaderT<PhysicalSocket>;
        using SocketWriter = SocketWriterT<PhysicalSocket>;
        using SocketAddress = typename PhysicalSocket::SocketAddress;

    public:
        SocketConnectionT() = delete;

    protected:
        SocketConnectionT(int fd,
                          const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                          const SocketAddress& localAddress,
                          const SocketAddress& remoteAddress,
                          const std::function<void()>& onDisconnect,
                          const utils::Timeval& readTimeout,
                          const utils::Timeval& writeTimeout,
                          std::size_t readBlockSize,
                          std::size_t writeBlockSize,
                          const utils::Timeval& terminateTimeout);

        ~SocketConnectionT() override;

    public:
        int getFd() const;

        void close() final;

        void shutdownRead() final;
        void shutdownWrite(bool forceClose) final;

        void setTimeout(const utils::Timeval& timeout) final;

        const SocketAddress& getLocalAddress() const final;
        const SocketAddress& getRemoteAddress() const final;

        std::size_t readFromPeer(char* junk, std::size_t junkLen) final;

        using Super::sendToPeer;
        void sendToPeer(const char* junk, std::size_t junkLen) final;

    private:
        void onReceivedFromPeer(std::size_t available) final;

    protected:
        void onWriteError(int errnum);
        void onReadError(int errnum);

        void onConnected();
        void onDisconnected();

    private:
        void onExit() final;

        void readTimeout() final;
        void writeTimeout() final;

        void unobservedEvent() final;

    protected: // must be callable from subclasses
        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        std::function<void()> onDisconnect;

        bool exitProcessed = false;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H
