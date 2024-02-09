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

#include "core/socket/stream/SocketReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cerrno>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    SocketReader::SocketReader(const std::string& instanceName,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               std::size_t blockSize,
                               const utils::Timeval& terminateTimeout)
        : core::eventreceiver::ReadEventReceiver(instanceName, timeout)
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

    void SocketReader::signalEvent([[maybe_unused]] int sigNum) { // Do nothing in case a signal was received
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
                const int errnum = errno;
                onStatus(errnum);
                disable();
            }
        } else {
            span();
        }

        return size;
    }

    void SocketReader::setBlockSize(std::size_t readBlockSize) {
        readBuffer.resize(readBlockSize);
        blockSize = readBlockSize;
    }

    std::size_t SocketReader::readFromPeer(char* junk, std::size_t junkLen) {
        const std::size_t maxReturn = std::min(junkLen, size);

        std::copy(readBuffer.data() + cursor, readBuffer.data() + cursor + maxReturn, junk);

        cursor += maxReturn;
        size -= maxReturn;

        return maxReturn;
    }

} // namespace core::socket::stream
