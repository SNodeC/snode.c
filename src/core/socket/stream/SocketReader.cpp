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

#include "core/socket/stream/SocketReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

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

    std::size_t SocketReader::getTotalRead() const {
        return totalRead;
    }

    std::size_t SocketReader::getTotalProcessed() const {
        return totalProcessed;
    }

    void SocketReader::readEvent() {
        const std::size_t available = doRead();

        if (available > 0) {
            onReceivedFromPeer(available);
        }
    }

    void SocketReader::signalEvent([[maybe_unused]] int sigNum) { // Do nothing in case a signal was received
    }

    ssize_t SocketReader::read(char* chunk, std::size_t chunkLen) {
        return core::system::recv(this->getRegisteredFd(), chunk, chunkLen, 0);
    }

    std::size_t SocketReader::doRead() {
        errno = 0;

        if (size == 0) {
            cursor = 0;

            ssize_t retRead = 0;
            if (!shutdownInProgress) {
                const std::size_t readLen = blockSize - size;
                retRead = read(readBuffer.data() + size, readLen);
            }
            if (retRead > 0) {
                totalRead += static_cast<std::size_t>(retRead);

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

    std::size_t SocketReader::readFromPeer(char* chunk, std::size_t chunkLen) {
        const std::size_t maxReturn = std::min(chunkLen, size);

        std::copy(readBuffer.data() + cursor, readBuffer.data() + cursor + maxReturn, chunk);

        cursor += maxReturn;
        size -= maxReturn;
        totalProcessed += maxReturn;

        return maxReturn;
    }

    void SocketReader::shutdownRead() {
        readBuffer.clear();

        size = 0;
        cursor = 0;

        shutdownInProgress = true;

        setTimeout(terminateTimeout);
    }

} // namespace core::socket::stream
