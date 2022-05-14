/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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
#include <cstddef> // for std::size_t
#include <functional>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketT>
    class SocketWriter
        : public core::eventreceiver::WriteEventReceiver
        , virtual public SocketT {
        SocketWriter() = delete;

    protected:
        using Socket = SocketT;

        explicit SocketWriter(const std::function<void(int)>& onError,
                              const utils::Timeval& timeout,
                              std::size_t blockSize,
                              const utils::Timeval& terminateTimeout)
            : core::eventreceiver::WriteEventReceiver("SocketWriter")
            , onError(onError)
            , terminateTimeout(terminateTimeout) {
            setBlockSize(blockSize);
            setTimeout(timeout);
            enable(Socket::getFd());
            suspend();
        }

        ~SocketWriter() override = default;

    private:
        virtual ssize_t write(const char* junk, std::size_t junkLen) = 0;

        void writeEvent() override = 0;

    protected:
        void setBlockSize(std::size_t writeBlockSize) {
            this->blockSize = writeBlockSize;
        }

        virtual void doWriteShutdown(const std::function<void(int)>& onShutdown) {
            errno = 0;

            Socket::shutdown(Socket::shutdown::WR);

            onShutdown(errno);
        }

        void sendToPeer(const char* junk, std::size_t junkLen) {
            if (!shutdownInProgress && !markShutdown) {
                writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);

                if (isSuspended()) {
                    resume();
                }
            }
        }

        void doWrite() {
            errno = 0;

            std::size_t writeLen = (writeBuffer.size() < blockSize) ? writeBuffer.size() : blockSize;
            ssize_t retWrite = write(writeBuffer.data(), writeLen);

            if (retWrite >= 0) {
                writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + retWrite);
            } else if (errno != EINTR /*&& errno != EAGAIN && errno != EWOULDBLOCK*/) {
                disable();
                onError(errno);
            }

            if (writeBuffer.empty()) {
                suspend();
                if (markShutdown) {
                    shutdown(onShutdown);
                }
            }
        }

        void shutdown(const std::function<void(int)>& onShutdown) {
            if (!shutdownInProgress) {
                this->onShutdown = onShutdown;
                if (isSuspended()) {
                    shutdownInProgress = true;
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
        std::size_t blockSize;

        bool markShutdown = false;

        bool shutdownInProgress = false;

        bool terminateInProgress = false;

        utils::Timeval terminateTimeout;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETWRITER_H
