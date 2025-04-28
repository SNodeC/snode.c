/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/pipe/PipeSource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_SEND_CHUNKSIZE
#define MAX_SEND_CHUNKSIZE 16384
#endif

namespace core::pipe {

    PipeSource::PipeSource(int fd)
        : core::eventreceiver::WriteEventReceiver("PipeSource fd = " + std::to_string(fd), 60) {
        if (!WriteEventReceiver::enable(fd)) {
            delete this;
        } else {
            WriteEventReceiver::suspend();
        }
    }

    PipeSource::~PipeSource() {
        close(getRegisteredFd());
    }

    void PipeSource::setOnError(const std::function<void(int)>& onError) {
        this->onError = onError;
    }

    void PipeSource::send(const char* chunk, std::size_t chunkLen) {
        writeBuffer.insert(writeBuffer.end(), chunk, chunk + chunkLen);

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
        const ssize_t ret = core::system::write(
            getRegisteredFd(), writeBuffer.data(), (writeBuffer.size() < MAX_SEND_CHUNKSIZE) ? writeBuffer.size() : MAX_SEND_CHUNKSIZE);

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

} // namespace core::pipe
