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

#include "core/file/FileReader.h"

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
        VLOG(0) << "############## WriteEvent()";
        if (fileReader != nullptr && writeBuffer.empty()) {
            const ssize_t count = fileReader->read(16384);

            VLOG(0) << "############## WriteEvent() - stream insert: " << count;

            if (count == 0) {
                fileReader = nullptr;
            }
        }

        doWrite();
    }

    void SocketWriter::signalEvent(int sigNum) {
        if (onSignal(sigNum)) {
            shutdownWrite([this]() -> void {
                SocketWriter::disable();
            });
        } else {
        }
    }

    void SocketWriter::doWrite() {
        VLOG(0) << "############## doWrite()";
        if (!writeBuffer.empty()) {
            const std::size_t writeLen = (writeBuffer.size() < blockSize) ? writeBuffer.size() : blockSize;
            const ssize_t retWrite = write(writeBuffer.data(), writeLen);

            //            VLOG(0) << "Sending Data: " << std::string(writeBuffer.data(), writeLen);

            if (retWrite > 0) {
                writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + retWrite);

                if (!writeBuffer.empty()) {
                    if (writeBuffer.capacity() > writeBuffer.size() * 2) {
                        writeBuffer.shrink_to_fit();
                    }
                }

                if (!isSuspended()) {
                    suspend();
                }
                span();
            } else if ((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) && isSuspended()) {
                resume();
            } else {
                onStatus(errno);
                disable();
            }
        } else {
            if (!isSuspended()) {
                suspend();
            }

            writeBuffer.shrink_to_fit();

            if (markShutdown) {
                LOG(TRACE) << "SocketWriter: Do delayed shutdown";

                shutdownWrite(onShutdown);
            }
        }
    }

    void SocketWriter::setBlockSize(std::size_t writeBlockSize) {
        this->blockSize = writeBlockSize;
    }

    void SocketWriter::sendToPeer(const char* junk, std::size_t junkLen) {
        VLOG(0) << "############## sendToPeer()";
        if (isEnabled()) {
            if (writeBuffer.empty()) {
                resume();
            }

            writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);
        } else {
            LOG(TRACE) << "SocketWriter: Send to peer while not enabled";
        }
    }

    void SocketWriter::streamToPeer(file::FileReader* fileReader) {
        VLOG(0) << "############## streamToPeer(): " << this->fileReader;

        this->fileReader = fileReader;

        if (fileReader->read(16384) == 0) {
            fileReader = nullptr;
        }
    }

    void SocketWriter::shutdownWrite(const std::function<void()>& onShutdown) {
        if (!shutdownInProgress) {
            SocketWriter::setTimeout(SocketWriter::terminateTimeout);

            SocketWriter::onShutdown = onShutdown;
            if (SocketWriter::writeBuffer.empty()) {
                doWriteShutdown(onShutdown);
                shutdownInProgress = true;
            } else {
                SocketWriter::markShutdown = true;
                LOG(TRACE) << "SocketWriter: Delay shutdown due to queued data";
            }
        }
    }

} // namespace core::socket::stream
