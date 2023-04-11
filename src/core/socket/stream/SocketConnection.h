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

#include "core/socket/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/SocketContext.h"

namespace core::socket::stream {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#include "log/Logger.h"

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
            SocketConnectionT::Descriptor::attach(fd);

            if (setSocketContext(socketContextFactory.get()) != nullptr) {
                SocketReader::enable(fd);
                SocketWriter::enable(fd);
                SocketWriter::suspend();
            }
        }

        ~SocketConnectionT() override {
            if (socketContext != nullptr) {
                onDisconnected();
                onDisconnect();

                delete socketContext;
            }
        }

    public:
        int getFd() const {
            return PhysicalSocket::getFd();
        }

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

        const SocketAddress& getRemoteAddress() const override {
            return remoteAddress;
        }

        const SocketAddress& getLocalAddress() const override {
            return localAddress;
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

        using Super::sendToPeer;

        void sendToPeer(const char* junk, std::size_t junkLen) final {
            if (newSocketContext == nullptr) {
                SocketWriter::sendToPeer(junk, junkLen);
            } else {
                VLOG(0) << "SendToPeer: OldSocketContext != nullptr: SocketContextSwitch in progress";
            }
        }

        void onWriteError(int errnum) {
            socketContext->onWriteError(errnum);
        }
        void onReadError(int errnum) {
            socketContext->onReadError(errnum);
        }

        void onConnected() {
            socketContext->onConnected();
        }

        void onDisconnected() {
            socketContext->onDisconnected();
        }

    private:
        void onReceivedFromPeer(std::size_t available) final {
            std::size_t consumed = socketContext->onReceivedFromPeer();

            if (newSocketContext != nullptr) { // Perform a pending SocketContextSwitch
                onDisconnected();
                delete socketContext;
                socketContext = newSocketContext;
                newSocketContext = nullptr;
                onConnected();
            }

            if (available != 0 && consumed == 0) {
                close();
            }
        }

        void readTimeout() final {
            close();
        }

        void writeTimeout() final {
            close();
        }

        void onExit() final {
            if (!exitProcessed) {
                socketContext->onExit();
                exitProcessed = true;
            }
        }

        void unobservedEvent() final {
            delete this;
        }

    protected: // must be callable from subclasses
        SocketAddress localAddress{};
        SocketAddress remoteAddress{};

        std::function<void()> onDisconnect;

        bool exitProcessed = false;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTION_H
