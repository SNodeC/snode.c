/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/socket/SocketConnection.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
    class SocketConnection
        : protected core::socket::SocketConnection
        , protected SocketReaderT
        , protected SocketWriterT {
        SocketConnection() = delete;

    protected:
        using Super = core::socket::SocketConnection;

        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;

        using SocketAddress = SocketAddressT;

        SocketConnection(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void()>& onConnect,
                         const std::function<void()>& onDisconnect,
                         const utils::Timeval& readTimeout,
                         const utils::Timeval& writeTimeout,
                         std::size_t readBlockSize,
                         std::size_t writeBlockSize,
                         const utils::Timeval& terminateTimeout)
            : Super(socketContextFactory)
            , SocketReader(
                  [this](int errnum) -> void {
                      onReadError(errnum);
                  },
                  readTimeout,
                  readBlockSize,
                  terminateTimeout)
            , SocketWriter(
                  [this](int errnum) -> void {
                      onWriteError(errnum);
                  },
                  writeTimeout,
                  writeBlockSize,
                  terminateTimeout)
            , localAddress(localAddress)
            , remoteAddress(remoteAddress)
            , onDisconnect(onDisconnect) {
            onConnect();
            onConnected();
        }

        ~SocketConnection() override {
            onDisconnected();
            onDisconnect();
        }

    public:
        using Super::getSocketContext;

        void close() final {
            SocketWriter::disable();
            SocketReader::disable();
        }

        void shutdownRead() final {
            SocketReader::shutdown();
        }

        void shutdownWrite() final {
            SocketWriter::shutdown();
        }

        void setTimeout(const utils::Timeval& timeout) final {
            SocketReader::setTimeout(timeout);
            SocketWriter::setTimeout(timeout);
        }

        const SocketAddress& getRemoteAddress() const {
            return remoteAddress;
        }

        const SocketAddress& getLocalAddress() const {
            return localAddress;
        }

        int getDescriptor() const override {
            return SocketConnection::getFd();
        }

        ssize_t readFromPeer(char* junk, std::size_t junkLen) final {
            ssize_t ret = 0;

            if (newSocketContext == nullptr) {
                ret = SocketReader::readFromPeer(junk, junkLen);
            } else {
                VLOG(0) << "ReadFromPeer: OldSocketContext != nullptr: SocketContextSwitch in progress";
            }

            return ret;
        }

        void sendToPeer(const char* junk, std::size_t junkLen) final {
            if (newSocketContext == nullptr) {
                SocketWriter::sendToPeer(junk, junkLen);
            } else {
                VLOG(0) << "SendToPeer: OldSocketContext != nullptr: SocketContextSwitch in progress";
            }
        }

        void sendToPeer(const std::string& data) final {
            sendToPeer(data.data(), data.size());
        }

    private:
        void readEvent() final {
            SocketReader::doRead();

            onReceiveFromPeer();
        }

        void writeEvent() final {
            SocketWriter::doWrite();
        }

        void unobservedEvent() final {
            delete this;
        }

        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        core::socket::SocketContext* newSocketContext = nullptr;

        std::function<void()> onDisconnect;

        template <typename ServerConfig, typename SocketConnection>
        friend class SocketAcceptor;

        template <typename ClientConfig, typename SocketConnection>
        friend class SocketConnector;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H
