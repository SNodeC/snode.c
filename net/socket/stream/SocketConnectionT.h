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
#include "net/socket/stream/SocketConnection.h"
#include "net/socket/stream/SocketContext.h"
#include "net/socket/stream/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
    class SocketConnectionT
        : public SocketConnection
        , public SocketReaderT
        , public SocketWriterT {
        SocketConnectionT() = delete;

    public:
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using SocketAddress = SocketAddressT;

    protected:
        SocketConnectionT(int fd,
                          const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                          const SocketAddress& localAddress,
                          const SocketAddress& remoteAddress,
                          const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                          const std::function<void()>& onDisconnect)
            : SocketReader([this](int errnum) -> void {
                socketContext->onReadError(errnum);
                SocketWriter::disable();
            })
            , SocketWriter([this](int errnum) -> void {
                socketContext->onWriteError(errnum);
                SocketReader::disable();
            })
            , socketContext(socketContextFactory->create(this))
            , localAddress(localAddress)
            , remoteAddress(remoteAddress)
            , onDisconnect(onDisconnect) {
            SocketConnectionT::attach(fd);
            SocketReader::enable(fd);
            SocketWriter::enable(fd);
            onConnect(localAddress, remoteAddress);
            socketContext->onConnected();
        }

        virtual ~SocketConnectionT() {
            socketContext->onDisconnected();
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

        void sendToPeer(const char* junk, std::size_t junkLen) override {
            SocketWriter::sendToPeer(junk, junkLen);
        }

        void sendToPeer(const std::string& data) override {
            sendToPeer(data.data(), data.size());
        }

        std::size_t readFromPeer(char* junk, std::size_t junkLen) override {
            return SocketReader::readFromPeer(junk, junkLen);
        }

        void close() final {
            //            SocketReader::disable();
            //            SocketReader::shutdown();
            SocketWriter::shutdown();
        }

        SocketContext* getSocketContext() override {
            return socketContext;
        }

        void switchSocketContext(SocketContextFactory* socketContextFactory) override {
            SocketContext* newSocketContext = socketContextFactory->create(this);

            if (newSocketContext != nullptr) {
                socketContext->onDisconnected();
                socketContext = newSocketContext;
                socketContext->onConnected();
            } else {
                VLOG(0) << "Switch socket context unsuccessull: new socket context not created";
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

        SocketContext* socketContext = nullptr;

        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        std::function<void()> onDisconnect;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETCONNECTION_H
