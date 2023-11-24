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

#include "core/socket/stream/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    SocketWriter::SocketWriter(const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               std::size_t blockSize,
                               const utils::Timeval& terminateTimeout)
        : core::eventreceiver::WriteEventReceiver("SocketWriter", timeout)
        , onStatus(onStatus)
        , terminateTimeout(terminateTimeout) {
        setBlockSize(blockSize);
    }

    void SocketWriter::writeEvent() {
        doWrite();
    }

    void SocketWriter::doWrite() {
        if (!writeBuffer.empty()) {
            std::size_t writeLen = (writeBuffer.size() < blockSize) ? writeBuffer.size() : blockSize;
            ssize_t retWrite = write(writeBuffer.data(), writeLen);

            if (retWrite > 0) {
                writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + retWrite);

                if (!isSuspended()) {
                    suspend();
                }
                if (!writeBuffer.empty()) {
                    if (writeBuffer.capacity() > writeBuffer.size() * 2) {
                        writeBuffer.shrink_to_fit();
                    }
                    span();
                } else {
                    writeBuffer.shrink_to_fit();

                    if (markShutdown) {
                        shutdownWrite(onShutdown);
                    }
                }
            } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                if (isSuspended()) {
                    resume();
                }
            } else {
                disable();
                onStatus(errno);
            }
        } else if (!isSuspended()) {
            suspend();
        }
    }

    void SocketWriter::setBlockSize(std::size_t writeBlockSize) {
        this->blockSize = writeBlockSize;
    }

    void SocketWriter::sendToPeer(const char* junk, std::size_t junkLen) {
        if (isEnabled()) {
            if (writeBuffer.empty()) {
                resume();
            }

            writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);
        }
    }

    void SocketWriter::terminate() {
        if (!terminateInProgress) {
            setTimeout(terminateTimeout);
            shutdownWrite([this](int errnum) -> void {
                if (errnum != 0) {
                    PLOG(TRACE) << "SocketWriter: SocketWriter::doWriteShutdown";
                }
                disable();
            });
            terminateInProgress = true;
        }
    }

} // namespace core::socket::stream