/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_SOCK_STREAM_SOCKETWRITER_H
#define NET_SOCKET_SOCK_STREAM_SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef> // for size_t
#include <functional>
#include <sys/types.h> // for ssize_t
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WriteEventReceiver.h"

#ifndef MAX_SEND_JUNKSIZE
#define MAX_SEND_JUNKSIZE 16384
#endif

namespace net::socket::stream {

    template <typename SocketT>
    class SocketWriter
        : public WriteEventReceiver
        , virtual public SocketT {
    protected:
        using Socket = SocketT;

        SocketWriter() = delete;

        explicit SocketWriter(const std::function<void(int errnum)>& onError)
            : onError(onError) {
        }

        void writeEvent() override {
            errno = 0;

            ssize_t ret = write(writeBuffer.data(), (writeBuffer.size() < MAX_SEND_JUNKSIZE) ? writeBuffer.size() : MAX_SEND_JUNKSIZE);

            if (ret > 0) {
                writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + ret);

                if (writeBuffer.empty()) {
                    WriteEventReceiver::disable();
                }
            } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                WriteEventReceiver::disable();
                onError(ret < 0 ? ret : errno);
            }
        }

        void enqueue(const char* junk, size_t junkLen) {
            writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);
            WriteEventReceiver::enable();
        }

        virtual ssize_t write(const char* junk, size_t junkSize) = 0;

    private:
        std::function<void(int errnum)> onError;

        std::vector<char> writeBuffer;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_SOCK_STREAM_SOCKETWRITER_H
