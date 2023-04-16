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

#include "core/socket/stream/SocketReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocket>
    SocketReader<PhysicalSocket>::SocketReader(const std::function<void(int)>& onError,
                                               const utils::Timeval& timeout,
                                               std::size_t blockSize,
                                               const utils::Timeval& terminateTimeout)
        : core::eventreceiver::ReadEventReceiver("SocketReader")
        , onError(onError)
        , terminateTimeout(terminateTimeout) {
        setBlockSize(blockSize);
        setTimeout(timeout);
    }

    template <typename PhysicalSocket>
    void SocketReader<PhysicalSocket>::readEvent() {
        std::size_t available = doRead();

        if (available > 0) {
            onReceivedFromPeer(available);
        }
    }

    template <typename PhysicalSocket>
    std::size_t SocketReader<PhysicalSocket>::doRead() {
        errno = 0;

        if (size == 0) {
            cursor = 0;

            std::size_t readLen = blockSize - size;
            ssize_t retRead = read(readBuffer.data() + size, readLen);

            if (retRead > 0) {
                size += static_cast<std::size_t>(retRead);

                if (!isSuspended()) {
                    suspend();
                }
                span();
            } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                if (isSuspended()) {
                    resume();
                }
            } else {
                LOG(TRACE) << "Received EOF";
                disable();
                onError(errno);
            }
        } else {
            span();
        }

        return size;
    }

    template <typename PhysicalSocket>
    void SocketReader<PhysicalSocket>::setBlockSize(std::size_t readBlockSize) {
        readBuffer.resize(readBlockSize);
        this->blockSize = readBlockSize;
    }

    template <typename PhysicalSocket>
    std::size_t SocketReader<PhysicalSocket>::readFromPeer(char* junk, std::size_t junkLen) {
        std::size_t maxReturn = std::min(junkLen, size);

        std::copy(readBuffer.data() + cursor, readBuffer.data() + cursor + maxReturn, junk);

        cursor += maxReturn;
        size -= maxReturn;

        return maxReturn;
    }

    template <typename PhysicalSocket>
    void SocketReader<PhysicalSocket>::shutdown() {
        if (!shutdownTriggered) {
            PhysicalSocket::shutdown(PhysicalSocket::SHUT::RD);
            shutdownTriggered = true;
        }
    }

    template <typename PhysicalSocket>
    void SocketReader<PhysicalSocket>::terminate() {
        if (!terminateInProgress) {
            setTimeout(terminateTimeout);
            // shutdown();
            // do not shutdown our read side because we try to do a full tcp shutdown sequence.
            // In case we do not receive a TCP-FIN (or equivalent depending on the protocol) the
            // connection is hard closed after "terminateTimeout".
            terminateInProgress = true;
        }
    }

} // namespace core::socket::stream
