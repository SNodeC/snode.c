/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "core/pipe/PipeSink.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_READ_CHUNKSIZE
#define MAX_READ_CHUNKSIZE 16384
#endif

namespace core::pipe {

    PipeSink::PipeSink(int fd, std::size_t maxBytesPerEvent, const utils::Timeval& timeout)
        : core::eventreceiver::ReadEventReceiver("PipeSink fd = " + std::to_string(fd), timeout)
        , maxBytesPerEvent(std::max<std::size_t>(maxBytesPerEvent, 1)) {
        ReadEventReceiver::enable(fd);
    }

    PipeSink::~PipeSink() {
        core::system::close(getRegisteredFd());
    }

    void PipeSink::readEvent() {
        std::array<char, MAX_READ_CHUNKSIZE> chunk{};
        std::size_t bytesRead = 0;

        while (bytesRead < maxBytesPerEvent) {
            const std::size_t bytesRemaining = maxBytesPerEvent - bytesRead;
            const std::size_t readSize = std::min(chunk.size(), bytesRemaining);
            const ssize_t ret = core::system::read(getRegisteredFd(), chunk.data(), readSize);

            if (ret > 0) {
                const std::size_t chunkLength = static_cast<std::size_t>(ret);
                bytesRead += chunkLength;

                if (onData) {
                    onData(chunk.data(), chunkLength);
                }

                if (!ReadEventReceiver::isEnabled()) {
                    return;
                }
            } else if (ret == 0) {
                ReadEventReceiver::disable();

                if (onEof) {
                    onEof();
                }
                return;
            } else if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else {
                const int errnum = errno;
                ReadEventReceiver::disable();

                if (onError) {
                    onError(errnum);
                }
                return;
            }
        }
    }

    void PipeSink::close() {
        if (ReadEventReceiver::isEnabled()) {
            ReadEventReceiver::disable();
        }
    }

    void PipeSink::setOnData(const std::function<void(const char*, std::size_t)>& onData) {
        this->onData = onData;
    }

    void PipeSink::setOnEof(const std::function<void()>& onEof) {
        this->onEof = onEof;
    }

    void PipeSink::setOnError(const std::function<void(int)>& onError) {
        this->onError = onError;
    }

    void PipeSink::setOnClosed(const std::function<void()>& onClosed) {
        this->onClosed = onClosed;
    }

    void PipeSink::unobservedEvent() {
        if (onClosed) {
            onClosed();
        }
        delete this;
    }

} // namespace core::pipe
