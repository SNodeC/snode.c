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

#ifndef NET_SOCKET_STREAM_SOCKETCONNECTION_H
#define NET_SOCKET_STREAM_SOCKETCONNECTION_H

#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"
#include "net/socket/stream/SocketContext.h"
#include "net/socket/stream/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
    class SocketConnection
        : public SocketConnectionBase
        , public SocketReaderT
        , public SocketWriterT {
        SocketConnection() = delete;

    public:
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using SocketAddress = SocketAddressT;

    protected:
        SocketConnection(int fd,
                         const std::shared_ptr<const SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                         const std::function<void()>& onDisconnect)
            : SocketReader([&socketContext = this->socketContext, this](int errnum) -> void {
                socketContext->onReadError(errnum);
                SocketWriter::disable();
            })
            , SocketWriter([&socketContext = this->socketContext, this](int errnum) -> void {
                socketContext->onWriteError(errnum);
                SocketReader::disable();
            })
            , localAddress(localAddress)
            , remoteAddress(remoteAddress)
            , onDisconnect(onDisconnect) {
            SocketConnection::attach(fd);
            SocketReader::enable(fd);
            SocketWriter::enable(fd);
            onConnect(localAddress, remoteAddress);
            socketContext = socketContextFactory->create(this);
            socketContext->onProtocolConnected();
        }

        virtual ~SocketConnection() {
            socketContext->onProtocolDisconnected();
            onDisconnect();
            delete socketContext;
        }

    public:
        void setTimeout(int timeout) override {
            SocketReader::setTimeout(timeout);
            SocketWriter::setTimeout(timeout);
        }

        const SocketAddress& getRemoteAddress() const {
            return remoteAddress;
        }

        const SocketAddress& getLocalAddress() const {
            return localAddress;
        }

        std::string getLocalAddressAsString() const override {
            return localAddress.toString();
        }

        std::string getRemoteAddressAsString() const override {
            return remoteAddress.toString();
        }

        void enqueue(const char* junk, std::size_t junkLen) override {
            SocketWriter::enqueue(junk, junkLen);
        }

        void enqueue(const std::string& data) override {
            enqueue(data.data(), data.size());
        }

        std::size_t doRead(char* junk, std::size_t junkLen) override {
            return SocketReader::doRead(junk, junkLen);
        }

        void close() final {
            SocketReader::disable();
            SocketWriter::disable();
        }

        SocketContext* getSocketContext() {
            return socketContext;
        }

        void switchSocketProtocol(const SocketContextFactory& socketContextFactory) override {
            SocketContext* newSocketContext = socketContextFactory.create(this);

            if (newSocketContext != nullptr) {
                socketContext->onProtocolDisconnected();
                socketContext = newSocketContext;
                socketContext->onProtocolConnected();
            }
        }

    private:
        void readEvent() override {
            socketContext->receiveFromPeer();
        }

        void writeEvent() override {
            SocketWriter::doWrite();
        }

        void unobserved() override {
            delete this;
        }

        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        std::function<void()> onDisconnect;

        SocketContext* socketContext = nullptr;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCONNECTION_H
