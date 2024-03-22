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

#ifndef CORE_SOCKET_STREAM_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_SOCKETCONNECTION_H

namespace core {
    namespace pipe {
        class Source;
    }
    namespace socket {
        class SocketAddress;
        namespace stream {
            class SocketContextFactory;
            class SocketContext;
        } // namespace stream
    }     // namespace socket
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketConnection {
    public:
        SocketConnection(const std::string& instanceName, const std::string& configuredServer);

        SocketConnection(SocketConnection&) = delete;
        SocketConnection(SocketConnection&&) = delete;

    protected:
        virtual ~SocketConnection();

        void setSocketContext(SocketContext* socketContext);

    public:
        void switchSocketContext(SocketContext* newSocketContext);

        virtual void sendToPeer(const char* chunk, std::size_t chunkLen) = 0;
        void sendToPeer(const std::string& data);
        void sentToPeer(const std::vector<uint8_t>& data);
        void sentToPeer(const std::vector<char>& data);

        virtual bool streamToPeer(core::pipe::Source* source) = 0;
        virtual void streamEof() = 0;

        virtual std::size_t readFromPeer(char* chunk, std::size_t chunkLen) = 0;

        virtual void shutdownRead() = 0;
        virtual void shutdownWrite(bool forceClose) = 0;

        const std::string& getInstanceName() const;
        const std::string& getConfiguredServer() const;

        virtual const core::socket::SocketAddress& getLocalAddress() const = 0;
        virtual const core::socket::SocketAddress& getRemoteAddress() const = 0;

        virtual void close() = 0;

        virtual void setTimeout(const utils::Timeval& timeout) = 0;

    protected:
        void connectSocketContext(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory);

        void disconnectCurrentSocketContext();

        core::socket::stream::SocketContext* socketContext = nullptr;
        core::socket::stream::SocketContext* newSocketContext = nullptr;

        std::string instanceName;
        std::string configuredServer;
    };

    template <typename PhysicalSocketT, typename SocketReaderT, typename SocketWriterT>
    class SocketConnectionT
        : public SocketConnection
        , protected SocketReaderT
        , protected SocketWriterT {
    protected:
        using Super = core::socket::stream::SocketConnection;

        using PhysicalSocket = PhysicalSocketT;
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using SocketAddress = typename PhysicalSocket::SocketAddress;

    public:
        SocketConnectionT() = delete;

    protected:
        SocketConnectionT(const std::string& instanceName,
                          PhysicalSocket&& physicalSocket,
                          const std::function<void()>& onDisconnect,
                          const std::string& configuredServer,
                          const SocketAddress& localAddress,
                          const SocketAddress& remoteAddress,
                          const utils::Timeval& readTimeout,
                          const utils::Timeval& writeTimeout,
                          std::size_t readBlockSize,
                          std::size_t writeBlockSize,
                          const utils::Timeval& terminateTimeout);

        ~SocketConnectionT() override;

    public:
        int getFd() const;

        void setTimeout(const utils::Timeval& timeout) final;

        const SocketAddress& getLocalAddress() const final;
        const SocketAddress& getRemoteAddress() const final;

        std::size_t readFromPeer(char* chunk, std::size_t chunkLen) final;

        using Super::sendToPeer;
        void sendToPeer(const char* chunk, std::size_t chunkLen) final;

        bool streamToPeer(core::pipe::Source* source) final;
        void streamEof() final;

        void shutdownRead() final;
        void shutdownWrite(bool forceClose) final;

        void close() final;

    protected:
        void doWriteShutdown(const std::function<void()>& onShutdown) override;

    private:
        PhysicalSocket physicalSocket;

        void onReceivedFromPeer(std::size_t available) final;

        void onWriteError(int errnum);
        void onReadError(int errnum);

        bool onSignal(int signum) final;

        void readTimeout() final;
        void writeTimeout() final;
        void unobservedEvent() final;

        std::function<void()> onDisconnect;

        SocketAddress localAddress{};
        SocketAddress remoteAddress{};
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H
