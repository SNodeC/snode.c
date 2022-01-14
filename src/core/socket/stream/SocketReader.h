/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstddef> // for std::size_t
#include <functional>
#include <sys/types.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_READ_JUNKSIZE
#define MAX_READ_JUNKSIZE 16384
#endif

#ifndef MAX_SHUTDOWN_TIMEOUT
#define MAX_SHUTDOWN_TIMEOUT 1
#endif

namespace core::socket::stream {

    template <typename SocketT>
    class SocketReader
        : public ReadEventReceiver
        , virtual public SocketT {
        SocketReader() = delete;

    public:
        using Socket = SocketT;

    protected:
        explicit SocketReader(const std::function<void(int)>& onError)
            : onError(onError) {
            enable(Socket::fd);
        }

        virtual ~SocketReader() = default;

    private:
        virtual ssize_t read(char* junk, std::size_t junkLen) = 0;

        void readEvent() override = 0;

    protected:
        virtual void doShutdown() {
            Socket::shutdown(Socket::shutdown::RD);
            if (isSuspended()) {
                resume();
            }
            shutdownTriggered = true;
        }

        ssize_t readFromPeer(char* junk, std::size_t junkLen) {
            std::size_t maxReturn = std::min(junkLen, size);

            std::copy(data + cursor, data + cursor + maxReturn, junk);

            cursor += maxReturn;
            size -= maxReturn;
            ssize_t ret = static_cast<ssize_t>(maxReturn);

            return ret;
        }

        void doRead() {
            errno = 0;

            if (size == 0) {
                cursor = 0;

                ssize_t retRead = 0;

                std::size_t readLen = MAX_READ_JUNKSIZE - size;
                retRead = read(data + size, readLen);

                int errnum = errno;
                //                PLOG(ERROR) << "Read++++++++++";
                errno = errnum;

                if (retRead > 0) {
                    size += static_cast<std::size_t>(retRead);
                } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                    disable(); // if we get a EOF or an other error disable the reader;
                    onError(errno);
                }
            }
        }

        bool continueReadImmediately() const override {
            return size > 0;
        }

        void shutdown() {
            if (!shutdownTriggered) {
                if (!isSuspended()) {
                    suspend();
                }
                doShutdown();
                setTimeout(MAX_SHUTDOWN_TIMEOUT);
            }
        }

        void terminate() override {
            setTimeout(MAX_SHUTDOWN_TIMEOUT);
            // shutdown(); // do not shutdown our read side because we try to do a full tcp shutdown sequence.
        }

    private:
        std::function<void(int)> onError;

        char data[MAX_READ_JUNKSIZE] = {};
        std::size_t size = 0;
        std::size_t cursor = 0;

        bool shutdownTriggered = false;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETREADER_H
