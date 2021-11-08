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

#include "net/pipe/PipeSource.h"

#include "net/system/unistd.h" // for write, ssize_t

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h> // for errno, EAGAIN, EINTR, EWOULDBLOCK
#include <string>  // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_SEND_JUNKSIZE
#define MAX_SEND_JUNKSIZE 16384
#endif

namespace net::pipe {

    PipeSource::PipeSource(int fd)
        : Descriptor(fd) {
        WriteEventReceiver::enable(fd);
        WriteEventReceiver::suspend();
    }

    void PipeSource::setOnError(const std::function<void(int errnum)>& onError) {
        this->onError = onError;
    }

    void PipeSource::send(const char* junk, std::size_t junkLen) {
        writeBuffer.insert(writeBuffer.end(), junk, junk + junkLen);

        if (WriteEventReceiver::isSuspended()) {
            WriteEventReceiver::resume();
        }
    }

    void PipeSource::send(const std::string& data) {
        send(data.data(), data.size());
    }

    void PipeSource::eof() {
        WriteEventReceiver::disable();
    }

    void PipeSource::writeEvent() {
        ssize_t ret = net::system::write(
            getFd(), writeBuffer.data(), (writeBuffer.size() < MAX_SEND_JUNKSIZE) ? writeBuffer.size() : MAX_SEND_JUNKSIZE);

        if (ret > 0) {
            writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + ret);

            if (writeBuffer.empty()) {
                WriteEventReceiver::suspend();
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            WriteEventReceiver::disable();

            if (onError) {
                onError(errno);
            }
        }
    }

    void PipeSource::unobserved() {
        delete this;
    }

} // namespace net::pipe
