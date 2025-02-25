/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"
#include "utils/system/signal.h"

#include <cstddef>
#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {
    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::SocketConnectionT(const std::string& instanceName,
                                                                                     PhysicalSocket&& physicalSocket,
                                                                                     const std::function<void()>& onDisconnect,
                                                                                     const std::string& configuredServer,
                                                                                     const SocketAddress& localAddress,
                                                                                     const SocketAddress& remoteAddress,
                                                                                     const utils::Timeval& readTimeout,
                                                                                     const utils::Timeval& writeTimeout,
                                                                                     std::size_t readBlockSize,
                                                                                     std::size_t writeBlockSize,
                                                                                     const utils::Timeval& terminateTimeout)
        : SocketConnection(instanceName, physicalSocket.getFd(), configuredServer)
        , SocketReader(
              instanceName + " [" + std::to_string(physicalSocket.getFd()) + "]",
              [this](int errnum) {
                  {
                      const utils::PreserveErrno pe(errnum);
                      if (errno == 0) {
                          LOG(TRACE) << connectionName << " OnReadError: EOF received";
                      } else {
                          PLOG(TRACE) << connectionName << " OnReadError";
                      }
                  }
                  SocketReader::disable();

                  onReadError(errnum);
              },
              readTimeout,
              readBlockSize,
              terminateTimeout)
        , SocketWriter(
              instanceName + " [" + std::to_string(physicalSocket.getFd()) + "]",
              [this](int errnum) {
                  {
                      const utils::PreserveErrno pe(errnum);
                      PLOG(TRACE) << connectionName << " OnWriteError";
                  }
                  SocketWriter::disable();

                  onWriteError(errnum);
              },
              writeTimeout,
              writeBlockSize,
              terminateTimeout)
        , physicalSocket(std::move(physicalSocket))
        , onDisconnect(onDisconnect)
        , localAddress(localAddress)
        , remoteAddress(remoteAddress) {
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
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::readFromPeer(char* chunk, std::size_t chunkLen) {
        std::size_t ret = 0;

        if (newSocketContext == nullptr) {
            ret = SocketReader::readFromPeer(chunk, chunkLen);
        } else {
            LOG(TRACE) << connectionName << " ReadFromPeer: New SocketContext != nullptr: SocketContextSwitch still in progress";
        }

        return ret;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::sendToPeer(const char* chunk, std::size_t chunkLen) {
        SocketWriter::sendToPeer(chunk, chunkLen);
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
        LOG(TRACE) << connectionName << ": Shutdown (RD)";

        SocketReader::shutdownRead();

        if (physicalSocket.shutdown(PhysicalSocket::SHUT::RD) == 0) {
            LOG(DEBUG) << connectionName << " Shutdown (RD): success";
        } else {
            PLOG(ERROR) << connectionName << " Shutdown (RD)";
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::shutdownWrite(bool forceClose) {
        if (!SocketWriter::shutdownInProgress) {
            LOG(TRACE) << connectionName << ": Stop writing";

            SocketWriter::shutdownWrite([forceClose, this]() {
                if (SocketWriter::isEnabled()) {
                    SocketWriter::disable();
                }
                if (forceClose && SocketReader::isEnabled()) {
                    close();
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

        setTimeout(SocketWriter::terminateTimeout);

        LOG(TRACE) << connectionName << ": Shutdown (WR)";

        if (physicalSocket.shutdown(PhysicalSocket::SHUT::WR) == 0) {
            LOG(DEBUG) << connectionName << " Shutdown (WR): success";
        } else {
            PLOG(ERROR) << connectionName << " Shutdown (WR)";
        }

        onShutdown();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::onReceivedFromPeer(std::size_t available) {
        std::size_t consumed = socketContext->onReceivedFromPeer();

        if (available != 0 && consumed == 0) {
            LOG(TRACE) << connectionName << ": Data available: " << available << " but nothing read";

            close();

            delete newSocketContext; // delete of nullptr is valid since C++14!
            newSocketContext = nullptr;
        } else if (newSocketContext != nullptr) { // Perform a pending SocketContextSwitch
            disconnectCurrentSocketContext();
            setSocketContext(newSocketContext);
            newSocketContext = nullptr;
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
                LOG(DEBUG) << connectionName << ": Shutting down due to signal '" << strsignal(signum) << "' (SIG"
                           << utils::system::sigabbrev_np(signum) << " [" << signum << "])";
                break;
            case SIGALRM:
                break;
        }

        return socketContext != nullptr ? socketContext->onSignal(signum) : true;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::readTimeout() {
        LOG(WARNING) << connectionName << ": Read timeout";
        close();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::writeTimeout() {
        LOG(WARNING) << connectionName << ": Write timeout";
        close();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter>::unobservedEvent() {
        disconnectCurrentSocketContext();

        onDisconnect();

        delete this;
    }

} // namespace core::socket::stream
