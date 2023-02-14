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

#ifndef CORE_SOCKET_STREAM_SOCKETWRITER_H
#define CORE_SOCKET_STREAM_SOCKETWRITER_H

#include "core/eventreceiver/WriteEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstddef>
#include <functional>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalSocketT>
    class SocketWriter
        : public core::eventreceiver::WriteEventReceiver
        , virtual public PhysicalSocketT {
    public:
        SocketWriter() = delete;

    protected:
        using PhysicalSocket = PhysicalSocketT;

        explicit SocketWriter(const std::function<void(int)>& onError,
                              const utils::Timeval& timeout,
                              std::size_t blockSize,
                              const utils::Timeval& terminateTimeout)
            : core::eventreceiver::WriteEventReceiver("SocketWriter")
            , onError(onError)
            , terminateTimeout(terminateTimeout) {
            setBlockSize(blockSize);
            setTimeout(timeout);
        }

        ~SocketWriter() override = default;

    private:
        virtual ssize_t write(const char* junk, std::size_t junkLen) = 0;

        void writeEvent() final {
            doWrite();
        }

        void doWrite() {
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
                            shutdown(onShutdown);
                        }
                    }
                } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    if (isSuspended()) {
                        resume();
                    }
                } else {
                    disable();
                    onError(errno);
                }
            } else if (!isSuspended()) {
                suspend();
            }
        }

    protected:
        void setBlockSize(std::size_t writeBlockSize) {
            this->blockSize = writeBlockSize;
        }

        void sendToPeer(const char* junk, std::size_t junkLen) {
            if (!shutdownInProgress && !markShutdown) {
                if (writeBuffer.empty()) {
                    resume();
                }

                writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);
            }
        }

        virtual void doWriteShutdown(const std::function<void(int)>& onShutdown) {
            errno = 0;

            LOG(TRACE) << "Do syscall shutdonw(WR)";

            PhysicalSocket::shutdown(PhysicalSocket::SHUT::WR);

            onShutdown(errno);
        }

        void shutdown(const std::function<void(int)>& onShutdown) {
            if (!shutdownInProgress) {
                this->onShutdown = onShutdown;
                if (writeBuffer.empty()) {
                    shutdownInProgress = true;
                    LOG(TRACE) << "Initiating shutdown process";
                    doWriteShutdown(onShutdown);
                } else {
                    markShutdown = true;
                }
            }
        }

        void terminate() override {
            if (!terminateInProgress) {
                setTimeout(terminateTimeout);
                shutdown([this]([[maybe_unused]] int errnum) -> void {
                    if (errnum != 0) {
                        PLOG(INFO) << "SocketWriter::doWriteShutdown";
                    }
                    disable();
                });
                terminateInProgress = true;
            }
        }

    private:
        std::function<void(int)> onError;
        std::function<void(int)> onShutdown;

        std::vector<char> writeBuffer;
        std::size_t blockSize = 0;

        bool markShutdown = false;
        bool shutdownInProgress = false;
        bool terminateInProgress = false;

        utils::Timeval terminateTimeout;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETWRITER_H
