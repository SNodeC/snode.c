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

namespace net::config {
    class ConfigInstance;
}

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
        SocketConnection(const net::config::ConfigInstance* config, int fd);
        SocketConnection(const SocketConnection&) = delete;

        virtual int getFd() const = 0;

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
        const std::string& getConnectionName() const;

        SocketContext* getSocketContext() const;

        virtual const core::socket::SocketAddress& getLocalAddress() const = 0;
        virtual const core::socket::SocketAddress& getRemoteAddress() const = 0;

        virtual void close() = 0;

        virtual void setTimeout(const utils::Timeval& timeout) = 0;

        virtual std::size_t getTotalSent() const = 0;
        virtual std::size_t getTotalQueued() const = 0;

        virtual std::size_t getTotalRead() const = 0;
        virtual std::size_t getTotalProcessed() const = 0;

        std::string getOnlineSince() const;
        std::string getOnlineDuration() const;

        const net::config::ConfigInstance* getConfig() const;

    private:
        static std::string timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
        static std::string
        durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                         const std::chrono::time_point<std::chrono::system_clock>& later = std::chrono::system_clock::now());

    protected:
        void connectSocketContext(const std::shared_ptr<SocketContextFactory>& socketContextFactory);

        void disconnectCurrentSocketContext();

        core::socket::stream::SocketContext* socketContext = nullptr;
        core::socket::stream::SocketContext* newSocketContext = nullptr;

        std::string instanceName;
        std::string connectionName;

        std::chrono::time_point<std::chrono::system_clock> onlineSinceTimePoint;

    private:
        const net::config::ConfigInstance* config;
    };

    template <typename ConfigT, typename PhysicalSocketT, typename SocketReaderT, typename SocketWriterT>
    class SocketConnectionT
        : public SocketConnection
        , protected SocketReaderT
        , protected SocketWriterT {
    protected:
        using Super = core::socket::stream::SocketConnection;

        using Config = ConfigT;
        using PhysicalSocket = PhysicalSocketT;
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using SocketAddress = typename PhysicalSocket::SocketAddress;

    public:
        SocketConnectionT() = delete;

    protected:
        SocketConnectionT(PhysicalSocket&& physicalSocket,
                          const std::function<void()>& onDisconnectm,
                          const std::shared_ptr<Config>& config);

        ~SocketConnectionT() override;

    public:
        int getFd() const final;

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

        Config& getConfig() const;

        std::size_t getTotalSent() const override;
        std::size_t getTotalQueued() const override;

        std::size_t getTotalRead() const override;
        std::size_t getTotalProcessed() const override;

    protected:
        void doWriteShutdown(const std::function<void()>& onShutdown) override;

        void onWriteError(int errnum);
        void onReadError(int errnum);

    private:
        void onReceivedFromPeer(std::size_t available) final;

        bool onSignal(int signum) final;

        void readTimeout() final;
        void writeTimeout() final;
        void unobservedEvent() final;

        PhysicalSocket physicalSocket;

        std::function<void()> onDisconnect;

        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        std::shared_ptr<Config> config;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H
