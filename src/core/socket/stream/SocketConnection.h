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

#include "core/socket/SocketConnection.h" // IWYU pragma: export
#include "core/socket/SocketContext.h"
#include "core/socket/SocketContextFactory.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketReaderT, typename SocketWriterT, typename SocketAddressT>
    class SocketConnection
        : public core::socket::SocketConnection
        , protected SocketReaderT
        , protected SocketWriterT {
        SocketConnection() = delete;

    private:
        using Super = core::socket::SocketConnection;

    public:
        using SocketReader = SocketReaderT;
        using SocketWriter = SocketWriterT;
        using SocketAddress = SocketAddressT;

    protected:
        SocketConnection(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void()>& onConnect,
                         const std::function<void()>& onDisconnect)
            : Super(socketContextFactory)
            , SocketReader([this](int errnum) -> void {
                onReadError(errnum);
            })
            , SocketWriter([this](int errnum) -> void {
                onWriteError(errnum);
            })
            , localAddress(localAddress)
            , remoteAddress(remoteAddress)
            , onDisconnect(onDisconnect) {
            SocketReader::enable(SocketConnection::getFd());
            SocketWriter::enable(SocketConnection::getFd());
            SocketReader::suspend();
            SocketWriter::suspend();
            onConnect();
            onConnected();
        }

        ~SocketConnection() override {
            onDisconnected();
            onDisconnect();
        }

    public:
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

    private:
        std::string getLocalAddressAsString() const final {
            return localAddress.toString();
        }

        std::string getRemoteAddressAsString() const final {
            return remoteAddress.toString();
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
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H
