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

#include "core/pipe/PipeSource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <cerrno>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_SEND_JUNKSIZE
#define MAX_SEND_JUNKSIZE 16384
#endif

namespace core::pipe {

    PipeSource::PipeSource(int fd)
        : core::eventreceiver::WriteEventReceiver("PipeSource fd = " + std::to_string(fd)) {
        WriteEventReceiver::enable(fd);
        WriteEventReceiver::suspend();
    }

    PipeSource::~PipeSource() {
        close(getRegisteredFd());
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
        ssize_t ret = core::system::write(
            getRegisteredFd(), writeBuffer.data(), (writeBuffer.size() < MAX_SEND_JUNKSIZE) ? writeBuffer.size() : MAX_SEND_JUNKSIZE);

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

    void PipeSource::unobservedEvent() {
        delete this;
    }

    void PipeSource::terminate() {
        WriteEventReceiver::terminate();
    }

} // namespace core::pipe
