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

#ifndef CORE_SOCKET_STREAM_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_SOCKETCONNECTION_H

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
    class SocketConnectionT
        : public SocketConnection
        , protected SocketReaderT
        , protected SocketWriterT {
        SocketConnectionT() = delete;

    public:
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using SocketAddress = SocketAddressT;

    protected:
        SocketConnectionT(const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                          const SocketAddress& localAddress,
                          const SocketAddress& remoteAddress,
                          const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                          const std::function<void()>& onDisconnect)
            : SocketConnection(socketContextFactory)
            , SocketReader([this](int errnum) -> void {
                if (SocketWriter::isEnabled()) {
                    SocketWriter::disable();
                }
                socketContext->onReadError(errnum);
            })
            , SocketWriter([this](int errnum) -> void {
                if (SocketReader::isEnabled()) {
                    SocketReader::disable();
                }
                socketContext->onWriteError(errnum);
            })
            , localAddress(localAddress)
            , remoteAddress(remoteAddress)
            , onDisconnect(onDisconnect) {
            SocketReader::enable(SocketConnectionT::getFd());
            SocketWriter::enable(SocketConnectionT::getFd());
            SocketReader::suspend();
            SocketWriter::suspend();
            onConnect(localAddress, remoteAddress);
            socketContext->onConnected();
        }

        virtual ~SocketConnectionT() {
            socketContext->onDisconnected();
            onDisconnect();
            delete socketContext;
        }

    public:
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

        ssize_t readFromPeer(char* junk, std::size_t junkLen) override {
            ssize_t ret = 0;

            if (newSocketContext == nullptr) {
                ret = SocketReader::readFromPeer(junk, junkLen);
            } else {
                VLOG(0) << "ReadFromPeer: OldSocketContext != nullptr: SocketContextSwitch in progress";
            }

            return ret;
        }

        void sendToPeer(const char* junk, std::size_t junkLen) override {
            if (newSocketContext == nullptr) {
                SocketWriter::sendToPeer(junk, junkLen);
            } else {
                VLOG(0) << "SendToPeer: OldSocketContext != nullptr: SocketContextSwitch in progress";
            }
        }

        void sendToPeer(const std::string& data) override {
            sendToPeer(data.data(), data.size());
        }

        void close() final {
            SocketWriter::shutdown();
        }

        void setTimeout(int timeout) override {
            SocketReader::setTimeout(timeout);
            SocketWriter::setTimeout(timeout);
        }

        SocketContext* switchSocketContext(SocketContextFactory* socketContextFactory) override {
            newSocketContext = socketContextFactory->create(this);

            if (newSocketContext == nullptr) {
                VLOG(0) << "Switch socket context unsuccessull: new socket context not created";
            }

            return newSocketContext;
        }

    private:
        void readEvent() override {
            socketContext->onReceiveFromPeer();

            if (newSocketContext != nullptr) { // Perform a pending SocketContextSwitch
                socketContext->onDisconnected();
                delete socketContext;
                socketContext = newSocketContext;
                newSocketContext = nullptr;
                socketContext->onConnected();
            }
        }

        void writeEvent() override {
            SocketWriter::doWrite();
        }

        void unobservedEvent() override {
            delete this;
        }

        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        SocketContext* newSocketContext = nullptr;

        std::function<void()> onDisconnect;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H