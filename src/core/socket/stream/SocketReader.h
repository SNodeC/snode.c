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

#ifndef CORE_SOCKET_STREAM_SOCKETREADER_H
#define CORE_SOCKET_STREAM_SOCKETREADER_H

#include "core/eventreceiver/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef> // for std::size_t
#include <functional>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketT>
    class SocketReader
        : public core::eventreceiver::ReadEventReceiver
        , virtual public SocketT {
        SocketReader() = delete;

    protected:
        using Socket = SocketT;

        explicit SocketReader(const std::function<void(int)>& onError,
                              const utils::Timeval& timeout,
                              std::size_t blockSize,
                              const utils::Timeval& terminateTimeout)
            : core::eventreceiver::ReadEventReceiver("SocketReader")
            , onError(onError)
            , terminateTimeout(terminateTimeout) {
            setBlockSize(blockSize);
            setTimeout(timeout);
            enable(Socket::getFd());
        }

        ~SocketReader() override = default;

    private:
        virtual ssize_t read(char* junk, std::size_t junkLen) = 0;

        void readEvent() override = 0;

    protected:
        void setBlockSize(std::size_t readBlockSize) {
            readBuffer.resize(readBlockSize);
            this->blockSize = readBlockSize;
        }

        ssize_t readFromPeer(char* junk, std::size_t junkLen) {
            std::size_t maxReturn = std::min(junkLen, size);

            std::copy(readBuffer.data() + cursor, readBuffer.data() + cursor + maxReturn, junk);

            cursor += maxReturn;
            size -= maxReturn;
            ssize_t ret = static_cast<ssize_t>(maxReturn);

            return ret;
        }

        void doRead() {
            errno = 0;

            if (size == 0) {
                cursor = 0;

                std::size_t readLen = blockSize - size;
                ssize_t retRead = read(readBuffer.data() + size, readLen);

                if (retRead > 0) {
                    size += static_cast<std::size_t>(retRead);

                    if (!isSuspended()) {
                        suspend();
                    }
                    publish();
                } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    if (isSuspended()) {
                        resume();
                    }
                } else {
                    disable();
                    onError(errno);
                }
            } else {
                publish();
            }
        }

        void shutdown() {
            if (!shutdownTriggered) {
                Socket::shutdown(Socket::shutdown::RD);
                shutdownTriggered = true;
            }
        }

        void terminate() override {
            if (!terminateInProgress) {
                setTimeout(terminateTimeout);
                // shutdown();
                // do not shutdown our read side because we try to do a full tcp shutdown sequence.
                // In case we do not receive a TCP-FIN (or equivalent depending on the protocol) the
                // connection is hard closed after "terminateTimeout".
                terminateInProgress = true;
            }
        }

    private:
        std::function<void(int)> onError;

        std::vector<char> readBuffer;
        std::size_t blockSize;

        std::size_t size = 0;
        std::size_t cursor = 0;

        bool shutdownTriggered = false;

        bool terminateInProgress = false;

        utils::Timeval terminateTimeout;

        int haveBeenRead = 0;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETREADER_H
