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

#ifndef NET_SOCKET_STREAM_SOCKETREADER_H
#define NET_SOCKET_STREAM_SOCKETREADER_H

#include "net/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef> // for std::size_t
#include <functional>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_READ_JUNKSIZE
#define MAX_READ_JUNKSIZE 16384
#endif

namespace net::socket::stream {

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
        }

        virtual ~SocketReader() = default;

        virtual void doShutdown() {
            Socket::shutdown(Socket::shutdown::RD);
        }

        void shutdown() {
            if (isSuspended() || !isEnabled()) {
                doShutdown();
            } else {
                markShutdown = true;
            }
        }

    private:
        virtual ssize_t read(char* junk, std::size_t junkLen) = 0;

    protected:
        ssize_t readFromPeer(char* junk, std::size_t junkLen) {
            ssize_t ret = 0;

            if (markShutdown) {
                doShutdown();
                markShutdown = false;
            } else {
                if (size == 0) {
                    coursor = 0;
                    ssize_t retRead = read(data, MAX_READ_JUNKSIZE);

                    if (retRead <= 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                            ReadEventReceiver::disable();
                            onError(getError());
                            ret = -1;
                        } else {
                            ret = 0;
                        }
                    } else {
                        size += static_cast<std::size_t>(retRead);
                    }
                }

                if (size > 0) {
                    std::size_t maxReturn = std::min(junkLen, size);

                    std::copy(data + coursor, data + coursor + maxReturn, junk);
                    coursor += maxReturn;
                    size -= maxReturn;

                    ret = static_cast<ssize_t>(maxReturn);
                }
            }

            return ret;
        }

        bool continueReadImmediately() override {
            return size > 0;
        }

    private:
        virtual int getError() = 0;

        std::function<void(int)> onError;

        std::size_t coursor = 0;
        std::size_t size = 0;

        char data[MAX_READ_JUNKSIZE];

        bool markShutdown = false;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETREADER_H
