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

#ifndef MAX_SEND_JUNKSIZE
#define MAX_SEND_JUNKSIZE 16384
#endif

namespace core::socket::stream {

    template <typename SocketT>
    class SocketWriter
        : public WriteEventReceiver
        , virtual public SocketT {
        SocketWriter() = delete;

    public:
        using Socket = SocketT;

    protected:
        explicit SocketWriter(const std::function<void(int)>& onError)
            : onError(onError) {
        }

        virtual ~SocketWriter() = default;

        void sendToPeer(const char* junk, std::size_t junkLen) {
            writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);

            if (isSuspended()) {
                resume();
            }
        }

        virtual void doShutdown() {
            Socket::shutdown(Socket::shutdown::WR);
            shut = true;
            resume();
            // disable(); // Normally this should be sufficient - but google-chrome
        }

        void shutdown() {
            if (!shut) {
                if (!markShutdown) {
                    if (isSuspended()) {
                        doShutdown();
                    } else {
                        markShutdown = true;
                    }
                }
            }
        }

    private:
        virtual ssize_t write(const char* junk, std::size_t junkLen) = 0;

        virtual void writeEvent() override = 0;

    protected:
        void terminate() override {
            shutdown();
        }

        void doWrite() {
            errno = 0;

            ssize_t retWrite = 0;

            std::size_t writeLen = (writeBuffer.size() < MAX_SEND_JUNKSIZE) ? writeBuffer.size() : MAX_SEND_JUNKSIZE;

            retWrite = write(writeBuffer.data(), writeLen);

            int errnum = getError();

            if (retWrite < 0 && errnum != EAGAIN && errnum != EWOULDBLOCK && errnum != EINTR) {
                disable();
                shut = true;

                onError(errnum);
            } else { // Remove on shutdown and on regular write
                writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + retWrite);
            }

            if (writeBuffer.empty()) {
                suspend();
                if (markShutdown && !shut) {
                    doShutdown();
                }
            }
        }

    private:
        virtual int getError() = 0;

        std::function<void(int)> onError;

        std::vector<char> writeBuffer;

        bool markShutdown = false;

        bool shut = false;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETWRITER_H
