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

#include "core/WriteEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef> // for std::size_t
#include <functional>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketT>
    class SocketWriter
        : public WriteEventReceiver
        , virtual public SocketT {
        SocketWriter() = delete;

    protected:
        using Socket = SocketT;

        explicit SocketWriter(const std::function<void(int)>& onError,
                              const utils::Timeval& timeout,
                              std::size_t blockSize,
                              const utils::Timeval& terminateTimeout)
            : onError(onError)
            , terminateTimeout(terminateTimeout) {
            setBlockSize(blockSize);
            setTimeout(timeout);
            enable(Socket::getFd());
            suspend();
        }

        virtual ~SocketWriter() = default;

    private:
        virtual ssize_t write(const char* junk, std::size_t junkLen) = 0;

        void writeEvent() override = 0;

    protected:
        void setBlockSize(std::size_t writeBlockSize) {
            this->blockSize = writeBlockSize;
        }

        virtual void doWriteShutdown() {
            Socket::shutdown(Socket::shutdown::WR);

            disable();
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

            ssize_t retWrite = -1;

            std::size_t writeLen = (writeBuffer.size() < blockSize) ? writeBuffer.size() : blockSize;
            retWrite = write(writeBuffer.data(), writeLen);

            if (retWrite >= 0) {
                writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + retWrite);
            } else if (errno != EINTR /*&& errno != EAGAIN && errno != EWOULDBLOCK*/) {
                disable();
                onError(errno);
            }

            if (writeBuffer.empty()) {
                suspend();
                if (markShutdown) {
                    shutdown();
                }
            }
        }

        void shutdown() {
            if (!shutdownInProgress) {
                if (isSuspended()) {
                    shutdownInProgress = true;
                    doWriteShutdown();
                } else {
                    markShutdown = true;
                }
            }
        }

        void terminate() override {
            if (!terminateInProgress) {
                setTimeout(terminateTimeout);
                shutdown();
                terminateInProgress = true;
            }
        }

    private:
        std::function<void(int)> onError;

        std::vector<char> writeBuffer;
        std::size_t blockSize;

        bool markShutdown = false;

        bool shutdownInProgress = false;

        bool terminateInProgress = false;

        utils::Timeval terminateTimeout;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETWRITER_H
