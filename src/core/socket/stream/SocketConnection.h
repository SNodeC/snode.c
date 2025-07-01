/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    } // namespace socket
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector> // IWYU pragma: keep

// IWYU pragma: no_include <format>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    class SocketConnection {
    public:
        SocketConnection(const std::string& instanceName, int fd, const std::string& configuredServer);
        SocketConnection(const SocketConnection&) = delete;

        virtual int getFd() const noexcept = 0;

    protected:
        virtual ~SocketConnection() noexcept;

        void setSocketContext(SocketContext* socketContext) noexcept;

    public:
        void switchSocketContext(SocketContext* newSocketContext) noexcept;

        virtual void sendToPeer(const char* chunk, std::size_t chunkLen) noexcept = 0;
        void sendToPeer(const std::string& data) noexcept;
        void sentToPeer(const std::vector<uint8_t>& data) noexcept;
        void sentToPeer(const std::vector<char>& data) noexcept;

        virtual bool streamToPeer(core::pipe::Source* source) noexcept = 0;
        virtual void streamEof() noexcept = 0;

        virtual std::size_t readFromPeer(char* chunk, std::size_t chunkLen) noexcept = 0;

        virtual void shutdownRead() noexcept = 0;
        virtual void shutdownWrite(bool forceClose) noexcept = 0;

        const std::string& getInstanceName() const noexcept;
        const std::string& getConnectionName() const noexcept;

        const std::string& getConfiguredServer() const noexcept;

        SocketContext* getSocketContext() const noexcept;

        virtual const core::socket::SocketAddress& getLocalAddress() const noexcept = 0;
        virtual const core::socket::SocketAddress& getRemoteAddress() const noexcept = 0;

        virtual void close() noexcept = 0;

        virtual void setTimeout(const utils::Timeval& timeout) noexcept = 0;

        virtual std::size_t getTotalSent() const noexcept = 0;
        virtual std::size_t getTotalQueued() const noexcept = 0;

        virtual std::size_t getTotalRead() const noexcept = 0;
        virtual std::size_t getTotalProcessed() const noexcept = 0;

        std::string getOnlineSince() const noexcept;
        std::string getOnlineDuration() const noexcept;

    private:
        static std::string timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint) noexcept;
        static std::string
        durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                         const std::chrono::time_point<std::chrono::system_clock>& later = std::chrono::system_clock::now()) noexcept;

    protected:
        void connectSocketContext(const std::shared_ptr<SocketContextFactory>& socketContextFactory) noexcept;

        void disconnectCurrentSocketContext() noexcept;

        core::socket::stream::SocketContext* socketContext = nullptr;
        core::socket::stream::SocketContext* newSocketContext = nullptr;

        std::string instanceName;
        std::string connectionName;

        std::string configuredServer;

        std::chrono::time_point<std::chrono::system_clock> onlineSinceTimePoint;
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
                          const utils::Timeval& terminateTimeout) noexcept;

        ~SocketConnectionT() noexcept override;

    public:
        int getFd() const noexcept final;

        void setTimeout(const utils::Timeval& timeout) noexcept final;

        const SocketAddress& getLocalAddress() const noexcept final;
        const SocketAddress& getRemoteAddress() const noexcept final;

        std::size_t readFromPeer(char* chunk, std::size_t chunkLen) noexcept final;

        using Super::sendToPeer;
        void sendToPeer(const char* chunk, std::size_t chunkLen) noexcept final;

        bool streamToPeer(core::pipe::Source* source) noexcept final;
        void streamEof() noexcept final;

        void shutdownRead() noexcept final;
        void shutdownWrite(bool forceClose) noexcept final;

        void close() noexcept final;

        std::size_t getTotalSent() const noexcept override;
        std::size_t getTotalQueued() const noexcept override;

        std::size_t getTotalRead() const noexcept override;
        std::size_t getTotalProcessed() const noexcept override;

    protected:
        void doWriteShutdown(const std::function<void()>& onShutdown) noexcept override;

        void onWriteError(int errnum) noexcept;
        void onReadError(int errnum) noexcept;

    private:
        void onReceivedFromPeer(std::size_t available) noexcept final;

        bool onSignal(int signum) noexcept final;

        void readTimeout() noexcept final;
        void writeTimeout() noexcept final;
        void unobservedEvent() noexcept final;

        PhysicalSocket physicalSocket;

        std::function<void()> onDisconnect;

        SocketAddress localAddress{};
        SocketAddress remoteAddress{};
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H
