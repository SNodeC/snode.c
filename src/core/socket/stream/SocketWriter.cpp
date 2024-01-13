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

#include "core/socket/stream/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/signal.h"

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

    void SocketWriter::signalEvent(int sigNum) {
        onSignal(sigNum);

        switch (sigNum) {
            case SIGINT:
                [[fallthrough]];
            case SIGTERM:
                [[fallthrough]];
            case SIGABRT:
                [[fallthrough]];
            case SIGHUP:
                if (!shutdownInProgress) {
                    SocketWriter::setTimeout(SocketWriter::terminateTimeout);
                    shutdownWrite([this]() -> void {
                        SocketWriter::disable();
                    });
                }
                break;
            case SIGALRM:
                break;
        }
    }

    void SocketWriter::doWrite() {
        if (!writeBuffer.empty()) {
            const std::size_t writeLen = (writeBuffer.size() < blockSize) ? writeBuffer.size() : blockSize;
            const ssize_t retWrite = write(writeBuffer.data(), writeLen);

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
                onStatus(errno);
                disable();
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

    void SocketWriter::shutdownWrite(const std::function<void()>& onShutdown) {
        SocketWriter::onShutdown = onShutdown;
        if (SocketWriter::writeBuffer.empty()) {
            doWriteShutdown(onShutdown);
        } else {
            SocketWriter::markShutdown = true;
        }
        shutdownInProgress = true;
    }

} // namespace core::socket::stream
