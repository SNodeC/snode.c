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
#include "utils/PreserveErrno.h"
#include "utils/system/signal.h"

#include <cstddef>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketConnectionT(const std::string& instanceName,
                                                                                     PhysicalSocket&& physicalSocket,
                                                                                     const std::function<void()>& onDisconnect,
                                                                                     const SocketAddress& localAddress,
                                                                                     const SocketAddress& remoteAddress,
                                                                                     const utils::Timeval& readTimeout,
                                                                                     const utils::Timeval& writeTimeout,
                                                                                     std::size_t readBlockSize,
                                                                                     std::size_t writeBlockSize,
                                                                                     const utils::Timeval& terminateTimeout)
        : SocketConnection(instanceName)
        , SocketReader(
              instanceName,
              [this](int errnum) -> void {
                  const utils::PreserveErrno pe(errnum);

                  if (errno == 0) {
                      LOG(TRACE) << this->instanceName << ": EOF received";
                  } else {
                      PLOG(TRACE) << this->instanceName << ": ReadError";
                  }

                  onReadError(errnum);
              },
              readTimeout,
              readBlockSize,
              terminateTimeout)
        , SocketWriter(
              instanceName,
              [this](int errnum) -> void {
                  const utils::PreserveErrno pe(errnum);

                  PLOG(TRACE) << this->instanceName << ": OnWriteError";

                  onWriteError(errnum);
              },
              writeTimeout,
              writeBlockSize,
              terminateTimeout)
        , physicalSocket(std::move(physicalSocket))
        , onDisconnect(onDisconnect)     // cppcheck-suppress selfInitialization
        , localAddress(localAddress)     // cppcheck-suppress selfInitialization
        , remoteAddress(remoteAddress) { // cppcheck-suppress selfInitialization
        if (!SocketReader::enable(this->physicalSocket.getFd())) {
            delete this;
        } else if (!SocketWriter::enable(this->physicalSocket.getFd())) {
            delete this;
        } else {
            SocketWriter::suspend();
        }
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
            LOG(TRACE) << instanceName << ": ReadFromPeer: OldSocketContext != nullptr: SocketContextSwitch still in progress";
        }

        return ret;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::sendToPeer(const char* junk, std::size_t junkLen) {
        SocketWriter::sendToPeer(junk, junkLen);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    bool SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::streamToPeer(core::pipe::Source* source) {
        return SocketWriter::streamToPeer(source);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::streamEof() {
        SocketWriter::streamEof();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownRead() {
        LOG(TRACE) << instanceName << ": Do syscall shutdown (RD)";

        physicalSocket.shutdown(PhysicalSocket::SHUT::RD);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownWrite(bool forceClose) {
        if (!SocketWriter::shutdownInProgress) {
            LOG(TRACE) << instanceName << ": Initiating shutdown process";

            SocketWriter::shutdownWrite([forceClose, this]() -> void {
                if (forceClose && SocketReader::isEnabled()) {
                    shutdownRead();
                }
                if (SocketWriter::isEnabled()) {
                    SocketWriter::disable();
                }
            });
        }
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
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::doWriteShutdown(const std::function<void()>& onShutdown) {
        errno = 0;

        LOG(TRACE) << instanceName << ": Do syscall shutdown (WR)";

        if (physicalSocket.shutdown(PhysicalSocket::SHUT::WR) != 0) {
            PLOG(TRACE) << instanceName << ": SocketWriter::doWriteShutdown";
        }

        onShutdown();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onReceivedFromPeer(std::size_t available) {
        std::size_t consumed = socketContext->onReceivedFromPeer();

        if (newSocketContext != nullptr) { // Perform a pending SocketContextSwitch
            disconnectCurrentSocketContext();
            setSocketContext(newSocketContext);
            newSocketContext = nullptr;
        }

        if (available != 0 && consumed == 0) {
            close();

            delete newSocketContext; // delete of nullptr is valid since C++14!
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
    bool SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onSignal(int signum) {
        switch (signum) {
            case SIGINT:
                [[fallthrough]];
            case SIGTERM:
                [[fallthrough]];
            case SIGABRT:
                [[fallthrough]];
            case SIGHUP:
                LOG(TRACE) << instanceName << ": Shutting down due to signal '" << strsignal(signum) << "' (SIG"
                           << utils::system::sigabbrev_np(signum) << " = " << signum << ")";
                break;
            case SIGALRM:
                break;
        }

        return socketContext != nullptr ? socketContext->onSignal(signum) : true;
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
        disconnectCurrentSocketContext();
        onDisconnect();

        delete this;
    }

} // namespace core::socket::stream
