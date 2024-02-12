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

#include "core/pipe/Source.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
            shutdownWrite([this]() -> void {
                SocketWriter::disable();
            });
        }
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
                onStatus(errno);
                disable();
            }
        } else {
            if (!isSuspended()) {
                suspend();
            }

            if (markShutdown) {
                shutdownWrite(onShutdown);
            } else if (source != nullptr) {
                source->resume();
            }
        }
    }

    void SocketWriter::setBlockSize(std::size_t writeBlockSize) {
        blockSize = writeBlockSize;
    }

    void SocketWriter::sendToPeer(const char* junk, std::size_t junkLen) {
        if (!shutdownInProgress && !markShutdown) {
            if (isEnabled()) {
                if (writePuffer.empty()) {
                    resume();
                }

                writePuffer.insert(writePuffer.end(), junk, junk + junkLen);

                if (source != nullptr && writePuffer.size() > 5 * blockSize) {
                    source->suspend();
                }
            } else {
                LOG(TRACE) << getName() << ": Send to peer while not enabled";
            }
        } else {
            LOG(TRACE) << getName() << ": sendToPeer() while shutdown in progress";
        }
    }

    bool SocketWriter::streamToPeer(core::pipe::Source* source) {
        bool success = false;

        if (!shutdownInProgress && !markShutdown) {
            if (isEnabled()) {
                if (source != nullptr) {
                    LOG(TRACE) << getName() << ": streamToPeer() started";

                    source->start();
                    success = true;
                } else {
                    LOG(TRACE) << getName() << ": streamToPeer() source is nullptr";
                }
            } else {
                LOG(TRACE) << getName() << ": streamToPeer() while not enabled";
            }
        } else {
            LOG(TRACE) << getName() << ": streamToPeer() while shutdown in progress";
        }

        this->source = source;

        return success;
    }

    void SocketWriter::streamEof() {
        this->source = nullptr;
    }

    void SocketWriter::shutdownWrite(const std::function<void()>& onShutdown) {
        if (!shutdownInProgress) {
            SocketWriter::setTimeout(SocketWriter::terminateTimeout);

            SocketWriter::onShutdown = onShutdown;
            if (SocketWriter::writePuffer.empty()) {
                doWriteShutdown(onShutdown);
                shutdownInProgress = true;
            } else {
                SocketWriter::markShutdown = true;
                LOG(TRACE) << getName() << ": Delay shutdown due to queued data";
            }
        }
    }

} // namespace core::socket::stream
