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

#ifndef NET_SOCKET_STREAM_SOCKETWRITER_H
#define NET_SOCKET_STREAM_SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef> // for std::size_t
#include <functional>
#include <sys/types.h> // for ssize_t
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "WriteEventReceiver.h"

#ifndef MAX_SEND_JUNKSIZE
#define MAX_SEND_JUNKSIZE 16384
#endif

namespace net::socket::stream {

    template <typename SocketT>
    class SocketWriter
        : public WriteEventReceiver
        , virtual public SocketT {
    public:
        using Socket = SocketT;

    protected:
        SocketWriter() = delete;

        explicit SocketWriter(const std::function<void(int errnum)>& onError)
            : onError(onError) {
        }

        virtual ~SocketWriter() = default;

        void enqueue(const char* junk, std::size_t junkLen) {
            writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);

            if (!WriteEventReceiver::isEnabled()) {
                WriteEventReceiver::enable(Socket::getFd());
            }
        }

        void shutdown() {
            if (!isEnabled()) {
                Socket::shutdown(Socket::shut::WR);
            } else {
                markShutdown = true;
            }
        }

    private:
        virtual ssize_t write(const char* junk, std::size_t junkLen) = 0;

        void writeEvent() override {
            errno = 0;

            ssize_t ret = write(writeBuffer.data(), (writeBuffer.size() < MAX_SEND_JUNKSIZE) ? writeBuffer.size() : MAX_SEND_JUNKSIZE);

            if (ret > 0) {
                writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + ret);

                if (writeBuffer.empty()) {
                    WriteEventReceiver::disable();
                    if (markShutdown) {
                        Socket::shutdown(Socket::shutdown::WR);
                    }
                }
            } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                WriteEventReceiver::disable();
                if (markShutdown) {
                    Socket::shutdown(Socket::shutdown::WR);
                }
                onError(getError());
            }
        }

        virtual int getError() = 0;

        std::function<void(int errnum)> onError;

        std::vector<char> writeBuffer;

        bool markShutdown = false;
    };

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_SOCKETWRITER_H
