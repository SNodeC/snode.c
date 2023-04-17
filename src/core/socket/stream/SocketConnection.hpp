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

#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketConnectionT(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
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
        if (setSocketContext(socketContextFactory.get()) != nullptr) {
            SocketReader::enable(getFd());
            SocketWriter::enable(getFd());
            SocketWriter::suspend();
        }
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::~SocketConnectionT() {
        if (socketContext != nullptr) {
            onDisconnected();
            onDisconnect();

            delete socketContext;
        }
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    int SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::getFd() const {
        return PhysicalSocket::getFd();
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::close() {
        if (SocketWriter::isEnabled()) {
            SocketWriter::disable();
        }
        if (SocketReader::isEnabled()) {
            SocketReader::disable();
        }
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownRead() {
        SocketReader::shutdown();
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownWrite(bool forceClose) {
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

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::setTimeout(const utils::Timeval& timeout) {
        SocketReader::setTimeout(timeout);
        SocketWriter::setTimeout(timeout);
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    const typename SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketAddress&
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::getLocalAddress() const {
        return localAddress;
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    const typename SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketAddress&
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::getRemoteAddress() const {
        return remoteAddress;
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::readFromPeer(char* junk, std::size_t junkLen) {
        std::size_t ret = 0;

        if (newSocketContext == nullptr) {
            ret = SocketReader::readFromPeer(junk, junkLen);
        } else {
            VLOG(0) << "ReadFromPeer: OldSocketContext != nullptr: SocketContextSwitch in progress";
        }

        return ret;
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::sendToPeer(const char* junk, std::size_t junkLen) {
        if (newSocketContext == nullptr) {
            SocketWriter::sendToPeer(junk, junkLen);
        } else {
            VLOG(0) << "SendToPeer: OldSocketContext != nullptr: SocketContextSwitch in progress";
        }
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onReceivedFromPeer(std::size_t available) {
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

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onWriteError(int errnum) {
        socketContext->onWriteError(errnum);
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onReadError(int errnum) {
        socketContext->onReadError(errnum);
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onConnected() {
        socketContext->onConnected();
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onDisconnected() {
        socketContext->onDisconnected();
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onExit() {
        if (!exitProcessed) {
            socketContext->onExit();
            exitProcessed = true;
        }
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::readTimeout() {
        close();
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::writeTimeout() {
        close();
    }

    template <typename PhysicalSocket,
              template <typename PhysicalSocketT>
              typename SocketReader,
              template <typename PhysicalSocketT>
              typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::unobservedEvent() {
        delete this;
    }

} // namespace core::socket::stream
