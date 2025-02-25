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

#include "core/socket/stream/SocketWriter.h"

#include "core/pipe/Source.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"
#include "log/Logger.h"

#include <cerrno>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    SocketWriter::SocketWriter(const std::string& instanceName,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               std::size_t blockSize,
                               const utils::Timeval& terminateTimeout)
        : core::eventreceiver::WriteEventReceiver(instanceName, timeout)
        , onStatus(onStatus)
        , blockSize(blockSize)
        , terminateTimeout(terminateTimeout) {
    }

    void SocketWriter::writeEvent() {
        doWrite();
    }

    void SocketWriter::signalEvent(int sigNum) {
        if (onSignal(sigNum)) {
            shutdownWrite([this]() {
                SocketWriter::disable();
            });
        }
    }

    ssize_t SocketWriter::write(const char* chunk, std::size_t chunkLen) {
        return core::system::send(this->getRegisteredFd(), chunk, chunkLen, MSG_NOSIGNAL);
    }

    void SocketWriter::doWrite() {
        if (!writePuffer.empty()) {
            const std::size_t writeLen = (writePuffer.size() < blockSize) ? writePuffer.size() : blockSize;
            const ssize_t retWrite = write(writePuffer.data(), writeLen);

            if (retWrite > 0) {
                writePuffer.erase(writePuffer.begin(), writePuffer.begin() + retWrite);

                if (writePuffer.capacity() > writePuffer.size() * 2) {
                    writePuffer.shrink_to_fit();
                }

                if (!isSuspended()) {
                    suspend();
                }
                span();
            } else if ((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) && isSuspended()) {
                resume();
            } else {
                onStatus(retWrite == 0 ? 0 : errno);
            }
        } else {
            if (!isSuspended()) {
                suspend();
            }

            if (markShutdown) {
                LOG(TRACE) << getName() << ": Shutdown restart";
                doWriteShutdown(onShutdown);
            } else if (source != nullptr) {
                source->resume();
            }
        }
    }

    void SocketWriter::setBlockSize(std::size_t writeBlockSize) {
        blockSize = writeBlockSize;
    }

    void SocketWriter::sendToPeer(const char* chunk, std::size_t chunkLen) {
        if (!shutdownInProgress && !markShutdown) {
            if (isEnabled()) {
                if (writePuffer.empty()) {
                    resume();
                }

                writePuffer.insert(writePuffer.end(), chunk, chunk + chunkLen);

                if (source != nullptr && writePuffer.size() > 5 * blockSize) {
                    source->suspend();
                }
            } else {
                LOG(WARNING) << getName() << ": Send while not enabled";
            }
        } else {
            LOG(WARNING) << getName() << ": Send while shutdown in progress";
        }
    }

    bool SocketWriter::streamToPeer(core::pipe::Source* source) {
        bool success = false;

        if (!shutdownInProgress && !markShutdown) {
            if (isEnabled()) {
                success = source != nullptr;

                if (success) {
                    LOG(TRACE) << getName() << ": Stream started";
                } else {
                    LOG(WARNING) << getName() << ": Stream source is nullptr";
                }
            } else {
                LOG(WARNING) << getName() << ": Stream while not enabled";
            }
        } else {
            LOG(WARNING) << getName() << ": Stream while shutdown in progress";
        }

        this->source = source;

        return success;
    }

    void SocketWriter::streamEof() {
        LOG(TRACE) << getName() << ": Stream EOF";
        this->source = nullptr;
    }

    void SocketWriter::shutdownWrite(const std::function<void()>& onShutdown) {
        if (!shutdownInProgress) {
            shutdownInProgress = true;

            SocketWriter::onShutdown = onShutdown;
            if (writePuffer.empty()) {
                LOG(TRACE) << getName() << ": Shutdown start";
                doWriteShutdown(onShutdown);
            } else {
                markShutdown = true;
                LOG(TRACE) << getName() << ": Shutdown delayed due to queued data";
            }
        }
    }

} // namespace core::socket::stream
