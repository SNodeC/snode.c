/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include <cstddef>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketConnectionT(const std::string& instanceName,
                                                                                     PhysicalSocket&& physicalSocket,
                                                                                     const std::function<void()>& onDisconnect,
                                                                                     const SocketAddress& localPeerAddress,
                                                                                     const SocketAddress& remotePeerAddress,
                                                                                     const utils::Timeval& readTimeout,
                                                                                     const utils::Timeval& writeTimeout,
                                                                                     std::size_t readBlockSize,
                                                                                     std::size_t writeBlockSize,
                                                                                     const utils::Timeval& terminateTimeout)
        : SocketConnection(instanceName)
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
        , physicalSocket(std::move(physicalSocket))
        , onDisconnect(onDisconnect)
        , localAddress(localPeerAddress)
        , remoteAddress(remotePeerAddress) {
        SocketReader::enable(this->physicalSocket.getFd());
        SocketWriter::enable(this->physicalSocket.getFd());
        SocketWriter::suspend();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::~SocketConnectionT() {
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::setTimeout(const utils::Timeval& timeout) {
        SocketReader::setTimeout(timeout);
        SocketWriter::setTimeout(timeout);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    int SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::getFd() const {
        return physicalSocket.getFd();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    const typename SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketAddress&
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::getLocalAddress() const {
        return localAddress;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    const typename SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketAddress&
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::getRemoteAddress() const {
        return remoteAddress;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::readFromPeer(char* junk, std::size_t junkLen) {
        std::size_t ret = 0;

        if (newSocketContext == nullptr) {
            ret = SocketReader::readFromPeer(junk, junkLen);
        } else {
            LOG(TRACE) << "SocketConnection: ReadFromPeer: OldSocketContext != nullptr: SocketContextSwitch still in progress";
        }

        return ret;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::sendToPeer(const char* junk, std::size_t junkLen) {
        if (newSocketContext == nullptr) {
            if (!shutdownInProgress && !SocketWriter::markShutdown) {
                SocketWriter::sendToPeer(junk, junkLen);
            }
        } else {
            LOG(TRACE) << "SocketConnection: SendToPeer: OldSocketContext != nullptr: SocketContextSwitch still in progress";
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownRead() {
        if (!shutdownTriggered) {
            physicalSocket.shutdown(PhysicalSocket::SHUT::RD);
            shutdownTriggered = true;
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownWrite(bool forceClose) {
        shutdownWrite([forceClose, this]() -> void {
            if (forceClose && SocketReader::isEnabled()) {
                SocketReader::disable();
            }
            if (SocketWriter::isEnabled()) {
                SocketWriter::disable();
            }
        });
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::close() {
        if (SocketWriter::isEnabled()) {
            SocketWriter::disable();
        }
        if (SocketReader::isEnabled()) {
            SocketReader::disable();
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    bool SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::getExitProcessed() {
        return exitProcessed;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::doWriteShutdown(const std::function<void()>& onShutdown) {
        errno = 0;

        LOG(TRACE) << "SocketConnection: Do syscall shutdonw (WR)";

        if (physicalSocket.shutdown(PhysicalSocket::SHUT::WR) != 0) {
            PLOG(TRACE) << "SocketConnection: SocketWriter::doWriteShutdown";
        }

        onShutdown();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onReceivedFromPeer(std::size_t available) {
        std::size_t consumed = socketContext->onReceivedFromPeer();

        if (newSocketContext != nullptr) { // Perform a pending SocketContextSwitch
            disconnected();
            socketContext = newSocketContext;
            newSocketContext = nullptr;
            connected(socketContext);
        }

        if (available != 0 && consumed == 0) {
            close();
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onWriteError(int errnum) {
        socketContext->onWriteError(errnum);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onReadError(int errnum) {
        socketContext->onReadError(errnum);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownWrite(const std::function<void()>& onShutdown) {
        if (!shutdownInProgress) {
            SocketWriter::onShutdown = onShutdown;
            if (SocketWriter::writeBuffer.empty()) {
                shutdownInProgress = true;
                LOG(TRACE) << "SocketWriter: Initiating shutdown process";
                doWriteShutdown(onShutdown);
            } else {
                SocketWriter::markShutdown = true;
            }
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onExit(int sig) {
        if (!exitProcessed) {
            socketContext->onExit(sig);
            exitProcessed = true;
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::readTimeout() {
        close();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::writeTimeout() {
        close();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::unobservedEvent() {
        disconnected();
        onDisconnect();

        delete this;
    }

} // namespace core::socket::stream
