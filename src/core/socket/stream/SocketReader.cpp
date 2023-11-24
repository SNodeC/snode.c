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

#include <algorithm>
#include <cerrno>
#include <functional>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    SocketReader::SocketReader(const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               std::size_t blockSize,
                               const utils::Timeval& terminateTimeout)
        : core::eventreceiver::ReadEventReceiver("SocketReader", timeout)
        , onStatus(onStatus)
        , terminateTimeout(terminateTimeout) {
        setBlockSize(blockSize);
    }

    void SocketReader::readEvent() {
        const std::size_t available = doRead();

        if (available > 0) {
            onReceivedFromPeer(available);
        }
    }

    std::size_t SocketReader::doRead() {
        errno = 0;

        if (size == 0) {
            cursor = 0;

            const std::size_t readLen = blockSize - size;
            const ssize_t retRead = read(readBuffer.data() + size, readLen);

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
                onStatus(errno);
                disable();
            }
        } else {
            span();
        }

        return size;
    }

    void SocketReader::setBlockSize(std::size_t readBlockSize) {
        readBuffer.resize(readBlockSize);
        this->blockSize = readBlockSize;
    }

    std::size_t SocketReader::readFromPeer(char* junk, std::size_t junkLen) {
        const std::size_t maxReturn = std::min(junkLen, size);

        std::copy(readBuffer.data() + cursor, readBuffer.data() + cursor + maxReturn, junk);

        cursor += maxReturn;
        size -= maxReturn;

        return maxReturn;
    }

    void SocketReader::terminate() {
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
