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

#include "core/pipe/PipeSource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <algorithm>
#include <cerrno>
#include <string>
#include <system_error>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_SEND_CHUNKSIZE
#define MAX_SEND_CHUNKSIZE 16384
#endif

namespace core::pipe {

    namespace {
        constexpr std::size_t MAX_BYTES_PER_EVENT = 256 * 1024;
    }

    PipeSource::PipeSource(int fd, std::size_t maxQueuedBytes, const utils::Timeval& timeout)
        : core::eventreceiver::WriteEventReceiver("PipeSource fd = " + std::to_string(fd), timeout)
        , maxQueuedBytes(maxQueuedBytes) {
        if (!WriteEventReceiver::enable(fd)) {
            throw std::system_error(errno != 0 ? errno : EIO, std::generic_category(), "unable to register PipeSource descriptor");
        }
        WriteEventReceiver::suspend();
    }

    PipeSource::~PipeSource() {
        closeDescriptor();
    }

    void PipeSource::setOnError(const std::function<void(int)>& onError) {
        this->onError = onError;
    }

    void PipeSource::setOnClosed(const std::function<void()>& onClosed) {
        this->onClosed = onClosed;
    }

    void PipeSource::setOnShutdown(const std::function<void(const core::ShutdownContext&)>& onShutdown) {
        shutdownCallback = onShutdown;
    }

    bool PipeSource::send(const char* chunk, std::size_t chunkLen) {
        const std::size_t queuedBytes = getQueuedBytes();
        if (closeWhenDrained || !WriteEventReceiver::isEnabled() || queuedBytes > maxQueuedBytes ||
            chunkLen > maxQueuedBytes - queuedBytes) {
            return false;
        }

        if (chunkLen == 0) {
            return true;
        }

        if (writeOffset > 0 && (writeOffset == writeBuffer.size() || writeOffset >= writeBuffer.size() / 2)) {
            writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + static_cast<std::ptrdiff_t>(writeOffset));
            writeOffset = 0;
        }

        writeBuffer.insert(writeBuffer.end(), chunk, chunk + chunkLen);

        if (chunkLen > 0 && WriteEventReceiver::isSuspended()) {
            WriteEventReceiver::resume();
        }

        return true;
    }

    bool PipeSource::send(const std::string& data) {
        return send(data.data(), data.size());
    }

    void PipeSource::eof() {
        closeWhenDrained = true;
        if (getQueuedBytes() == 0) {
            close();
        }
    }

    void PipeSource::close() {
        closeWhenDrained = true;
        writeBuffer.clear();
        writeOffset = 0;
        if (WriteEventReceiver::isEnabled()) {
            WriteEventReceiver::disable();
        }
    }

    std::size_t PipeSource::getQueuedBytes() const noexcept {
        return writeBuffer.size() - writeOffset;
    }

    void PipeSource::writeEvent() {
        std::size_t bytesWritten = 0;

        while (getQueuedBytes() > 0 && bytesWritten < MAX_BYTES_PER_EVENT) {
            const std::size_t writeSize =
                std::min({getQueuedBytes(), static_cast<std::size_t>(MAX_SEND_CHUNKSIZE), MAX_BYTES_PER_EVENT - bytesWritten});
            const ssize_t ret = core::system::write(getRegisteredFd(), writeBuffer.data() + writeOffset, writeSize);

            if (ret > 0) {
                const std::size_t written = static_cast<std::size_t>(ret);
                writeOffset += written;
                bytesWritten += written;
            } else if (ret == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else if (errno == EINTR) {
                continue;
            } else {
                const int errnum = errno;
                WriteEventReceiver::disable();

                if (onError) {
                    onError(errnum);
                }
                return;
            }
        }

        if (getQueuedBytes() == 0) {
            writeBuffer.clear();
            writeOffset = 0;
            if (closeWhenDrained) {
                WriteEventReceiver::disable();
            } else {
                WriteEventReceiver::suspend();
            }
        }
    }

    void PipeSource::unobservedEvent() {
        closeDescriptor();
        if (onClosed) {
            onClosed();
        }
        delete this;
    }

    void PipeSource::destruct() {
        delete this;
    }

    void PipeSource::shutdownEvent(const core::ShutdownContext& context) {
        if (shutdownCallback) {
            shutdownCallback(context);
        } else {
            close();
        }
    }

    void PipeSource::closeDescriptor() noexcept {
        if (!descriptorClosed) {
            descriptorClosed = true;
            core::system::close(getRegisteredFd());
        }
    }

} // namespace core::pipe
