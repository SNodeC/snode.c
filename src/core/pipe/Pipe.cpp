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

#include "core/pipe/Pipe.h"

#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <system_error>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    namespace {
        void closeDescriptor(int& fd) noexcept {
            const int descriptor = std::exchange(fd, -1);
            if (descriptor >= 0) {
                const int savedErrno = errno;
                core::system::close(descriptor);
                errno = savedErrno;
            }
        }

        void makeNonBlocking(int fd) {
            int flags = -1;
            do {
                flags = ::fcntl(fd, F_GETFL);
            } while (flags < 0 && errno == EINTR);

            if (flags < 0) {
                throw std::system_error(errno, std::generic_category(), "unable to inspect pipe endpoint flags");
            }

            if ((flags & O_NONBLOCK) == 0) {
                int result = -1;
                do {
                    result = ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
                } while (result < 0 && errno == EINTR);

                if (result < 0) {
                    throw std::system_error(errno, std::generic_category(), "unable to make pipe endpoint nonblocking");
                }
            }
        }
    } // namespace

    Pipe::Pipe() noexcept
        : Pipe(O_CLOEXEC) {
    }

    Pipe::Pipe(int flags) noexcept {
        int descriptors[2] = {-1, -1};
        if (core::system::pipe2(descriptors, flags) != 0) {
            error = errno;
            return;
        }

        readFd = descriptors[0];
        writeFd = descriptors[1];
    }

    Pipe::Pipe(Pipe&& pipe) noexcept
        : readFd(std::exchange(pipe.readFd, -1))
        , writeFd(std::exchange(pipe.writeFd, -1))
        , error(std::exchange(pipe.error, 0)) {
    }

    Pipe::~Pipe() {
        closeRead();
        closeWrite();
    }

    Pipe& Pipe::operator=(Pipe&& pipe) noexcept {
        if (this != &pipe) {
            closeRead();
            closeWrite();
            readFd = std::exchange(pipe.readFd, -1);
            writeFd = std::exchange(pipe.writeFd, -1);
            error = std::exchange(pipe.error, 0);
        }
        return *this;
    }

    Pipe::Pipe(const std::function<void(PipeSource&, PipeSink&)>& onSuccess, const std::function<void(int)>& onError)
        : Pipe(O_NONBLOCK | O_CLOEXEC) {
        if (!hasReadFd() || !hasWriteFd()) {
            onError(error);
            return;
        }

        PipeSource* pipeSource = releaseWriteAsSource();
        if (pipeSource == nullptr) {
            onError(error != 0 ? error : EIO);
            return;
        }
        PipeSink* pipeSink = nullptr;
        try {
            pipeSink = releaseReadAsSink();
            if (pipeSink == nullptr) {
                pipeSource->close();
                onError(error != 0 ? error : EIO);
                return;
            }
            onSuccess(*pipeSource, *pipeSink);
        } catch (...) {
            pipeSource->close();
            if (pipeSink != nullptr) {
                pipeSink->close();
            }
            throw;
        }
    }

    bool Pipe::isValid() const noexcept {
        return hasReadFd() || hasWriteFd();
    }

    bool Pipe::hasReadFd() const noexcept {
        return readFd >= 0;
    }

    bool Pipe::hasWriteFd() const noexcept {
        return writeFd >= 0;
    }

    int Pipe::getError() const noexcept {
        return error;
    }

    int Pipe::getReadFd() const noexcept {
        return readFd;
    }

    int Pipe::getWriteFd() const noexcept {
        return writeFd;
    }

    int Pipe::releaseReadFd() noexcept {
        return std::exchange(readFd, -1);
    }

    int Pipe::releaseWriteFd() noexcept {
        return std::exchange(writeFd, -1);
    }

    void Pipe::closeRead() noexcept {
        closeDescriptor(readFd);
    }

    void Pipe::closeWrite() noexcept {
        closeDescriptor(writeFd);
    }

    PipeSink* Pipe::releaseReadAsSink() {
        return releaseReadAsSink(PipeSink::DEFAULT_MAX_BYTES_PER_EVENT, utils::Timeval({60, 0}));
    }

    PipeSink* Pipe::releaseReadAsSink(std::size_t maxBytesPerEvent, const utils::Timeval& timeout) {
        if (!hasReadFd()) {
            return nullptr;
        }

        makeNonBlocking(readFd);
        const int descriptor = releaseReadFd();
        try {
            return new PipeSink(descriptor, maxBytesPerEvent, timeout);
        } catch (...) {
            readFd = descriptor;
            throw;
        }
    }

    PipeSource* Pipe::releaseWriteAsSource() {
        return releaseWriteAsSource(PipeSource::DEFAULT_MAX_QUEUED_BYTES, utils::Timeval({60, 0}));
    }

    PipeSource* Pipe::releaseWriteAsSource(std::size_t maxQueuedBytes, const utils::Timeval& timeout) {
        if (!hasWriteFd()) {
            return nullptr;
        }

        makeNonBlocking(writeFd);
        const int descriptor = releaseWriteFd();
        try {
            return new PipeSource(descriptor, maxQueuedBytes, timeout);
        } catch (...) {
            writeFd = descriptor;
            throw;
        }
    }

} // namespace core::pipe
