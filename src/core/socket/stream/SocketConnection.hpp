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

    template <typename SocketAddress, typename PhysicalSocket, typename Config>
    SocketAddress getLocalSocketAddress(PhysicalSocket& physicalSocket, Config& config) {
        typename SocketAddress::SockAddr localSockAddr;
        typename SocketAddress::SockLen localSockAddrLen = sizeof(typename SocketAddress::SockAddr);

        SocketAddress localPeerAddress;
        if (physicalSocket.getSockName(localSockAddr, localSockAddrLen) == 0) {
            try {
                localPeerAddress = config->Local::getSocketAddress(localSockAddr, localSockAddrLen);
                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                           << "  PeerAddress (local): " << localPeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                             << "  PeerAddress (local): " << badSocketAddress.what();
            }
        } else {
            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                          << " PeerAddress (local) not retrievable";
        }

        return localPeerAddress;
    }

    template <typename SocketAddress, typename PhysicalSocket, typename Config>
    SocketAddress getRemoteSocketAddress(PhysicalSocket& physicalSocket, Config& config) {
        typename SocketAddress::SockAddr remoteSockAddr;
        typename SocketAddress::SockLen remoteSockAddrLen = sizeof(typename SocketAddress::SockAddr);

        SocketAddress remotePeerAddress;
        if (physicalSocket.getPeerName(remoteSockAddr, remoteSockAddrLen) == 0) {
            try {
                remotePeerAddress = config->Remote::getSocketAddress(remoteSockAddr, remoteSockAddrLen);
                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                           << "  PeerAddress (remote): " << remotePeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                             << "  PeerAddress (remote): " << badSocketAddress.what();
            }
        } else {
            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                          << " PeerAddress (remote) not retrievble";
        }

        return remotePeerAddress;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::SocketConnectionT(PhysicalSocket&& physicalSocket,
                                                                                             const std::function<void()>& onDisconnectm,
                                                                                             const std::shared_ptr<Config>& config)
        : SocketConnection(config.get(), physicalSocket.getFd())
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
              config->getReadTimeout(),
              config->getReadBlockSize(),
              config->getTerminateTimeout())
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
              config->getWriteTimeout(),
              config->getWriteBlockSize(),
              config->getTerminateTimeout())
        , physicalSocket(std::move(physicalSocket))
        , onDisconnect(onDisconnectm)
        , localAddress(getLocalSocketAddress<SocketAddress>(this->physicalSocket, config))
        , remoteAddress(getRemoteSocketAddress<SocketAddress>(this->physicalSocket, config))
        , config(config) {
        if (!SocketReader::enable(this->physicalSocket.getFd())) {
            delete this;
        } else if (!SocketWriter::enable(this->physicalSocket.getFd())) {
            delete this;
        } else {
            SocketWriter::suspend();
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::~SocketConnectionT() {
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::setTimeout(const utils::Timeval& timeout) {
        SocketReader::setTimeout(timeout);
        SocketWriter::setTimeout(timeout);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    int SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getFd() const {
        return physicalSocket.getFd();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    const typename SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::SocketAddress&
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getLocalAddress() const {
        return localAddress;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    const typename SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::SocketAddress&
    SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getRemoteAddress() const {
        return remoteAddress;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::readFromPeer(char* chunk, std::size_t chunkLen) {
        std::size_t ret = 0;

        if (newSocketContext == nullptr) {
            ret = SocketReader::readFromPeer(chunk, chunkLen);
        } else {
            LOG(TRACE) << connectionName << " ReadFromPeer: New SocketContext != nullptr: SocketContextSwitch still in progress";
        }

        return ret;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::sendToPeer(const char* chunk, std::size_t chunkLen) {
        SocketWriter::sendToPeer(chunk, chunkLen);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    bool SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::streamToPeer(core::pipe::Source* source) {
        return SocketWriter::streamToPeer(source);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::streamEof() {
        SocketWriter::streamEof();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::shutdownRead() {
        LOG(TRACE) << connectionName << ": Shutdown (RD)";

        SocketReader::shutdownRead();

        if (physicalSocket.shutdown(PhysicalSocket::SHUT::RD) == 0) {
            LOG(DEBUG) << connectionName << " Shutdown (RD): success";
        } else {
            PLOG(ERROR) << connectionName << " Shutdown (RD)";
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::shutdownWrite(bool forceClose) {
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

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::close() {
        if (SocketWriter::isEnabled()) {
            SocketWriter::disable();
        }
        if (SocketReader::isEnabled()) {
            SocketReader::disable();
        }
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    Config& SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getConfig() const {
        return *config;
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getTotalSent() const {
        return SocketWriter::getTotalSent();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getTotalQueued() const {
        return SocketWriter::getTotalQueued();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getTotalRead() const {
        return SocketReader::getTotalRead();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    std::size_t SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::getTotalProcessed() const {
        return SocketReader::getTotalProcessed();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::doWriteShutdown(const std::function<void()>& onShutdown) {
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

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::onReceivedFromPeer(std::size_t available) {
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

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::onWriteError(int errnum) {
        socketContext->onWriteError(errnum);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::onReadError(int errnum) {
        socketContext->onReadError(errnum);
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    bool SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::onSignal(int signum) {
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

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::readTimeout() {
        LOG(WARNING) << connectionName << ": Read timeout";
        close();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::writeTimeout() {
        LOG(WARNING) << connectionName << ": Write timeout";
        close();
    }

    template <typename PhysicalSocket, typename SocketReader, typename SocketWriter, typename Config>
    void SocketConnectionT<PhysicalSocket, SocketReader, SocketWriter, Config>::unobservedEvent() {
        disconnectCurrentSocketContext();

        onDisconnect();

        delete this;
    }

} // namespace core::socket::stream
