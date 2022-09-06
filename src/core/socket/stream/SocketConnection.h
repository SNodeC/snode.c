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
#include "core/socket/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketT, template <typename Socket> class SocketReaderT, template <typename Socket> class SocketWriterT>
    class SocketConnection
        : protected core::socket::SocketConnection
        , protected SocketReaderT<SocketT>
        , protected SocketWriterT<SocketT> {
    protected:
        using Super = core::socket::SocketConnection;

        using Socket = SocketT;
        using SocketReader = SocketReaderT<Socket>;
        using SocketWriter = SocketWriterT<Socket>;

        using SocketAddress = typename Socket::SocketAddress;

    public:
        SocketConnection() = delete;

    protected:
        SocketConnection(int fd,
                         const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void()>& onConnect,
                         const std::function<void()>& onDisconnect,
                         const utils::Timeval& readTimeout,
                         const utils::Timeval& writeTimeout,
                         std::size_t readBlockSize,
                         std::size_t writeBlockSize,
                         const utils::Timeval& terminateTimeout)
            : SocketReader(
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
            SocketConnection::Descriptor::open(fd);

            setSocketContext(socketContextFactory.get());

            SocketReader::enable(fd);
            SocketWriter::enable(fd);
            SocketWriter::suspend();

            onConnect();
        }

        ~SocketConnection() override {
            onDisconnected();
            onDisconnect();
        }

    public:
        using Super::getSocketContext;

        void close() final {
            if (SocketWriter::isEnabled()) {
                SocketWriter::disable();
            }
            if (SocketReader::isEnabled()) {
                SocketReader::disable();
            }
        }

        void shutdownRead() final {
            SocketReader::shutdown();
        }

        void shutdownWrite(bool forceClose) final {
            SocketWriter::shutdown([forceClose, this](int errnum) -> void {
                if (errnum != 0) {
                    PLOG(INFO) << "SocketWriter::doWriteShutdown";
                }
                if (forceClose) {
                    close();
                } else if (SocketWriter::isEnabled()) {
                    SocketWriter::disable();
                }
            });
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

        Socket& getSocket() override {
            return *this;
        }

        std::size_t readFromPeer(char* junk, std::size_t junkLen) final {
            std::size_t ret = 0;

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
        std::size_t onReceiveFromPeer() final {
            std::size_t ret = socketContext->onReceiveFromPeer();

            if (newSocketContext != nullptr) { // Perform a pending SocketContextSwitch
                onDisconnected();
                delete socketContext;
                socketContext = newSocketContext;
                newSocketContext = nullptr;
                onConnected();
            }

            return ret;
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
