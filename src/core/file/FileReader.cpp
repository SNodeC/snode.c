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

#include "core/file/FileReader.h"

#include "core/State.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <cerrno>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

constexpr int MF_READSIZE = 16384;

namespace core::file {

    namespace {

        class DescriptorGuard {
        public:
            explicit DescriptorGuard(int fd) noexcept
                : fd(fd) {
            }

            DescriptorGuard(const DescriptorGuard&) = delete;
            DescriptorGuard& operator=(const DescriptorGuard&) = delete;

            ~DescriptorGuard() {
                if (fd >= 0) {
                    core::system::close(fd);
                }
            }

            int get() const noexcept {
                return fd;
            }

            int release() noexcept {
                return std::exchange(fd, -1);
            }

        private:
            int fd;
        };

        bool requiresMode(int flags) {
            if ((flags & O_CREAT) != 0) {
                return true;
            }

#ifdef O_TMPFILE
            if ((flags & O_TMPFILE) == O_TMPFILE) {
                return true;
            }
#endif

            return false;
        }

    } // namespace

    FileReader::FileReader(int fd, const std::string& name, std::size_t pufferSize, int openErrno)
        : core::Descriptor(fd)
        , EventReceiver(name)
        , pufferSize(pufferSize)
        , openErrno(openErrno) {
    }

    FileReader* FileReader::create(int fd, const std::string& name) {
        DescriptorGuard descriptor(fd);
        std::unique_ptr<FileReader> fileReader(new FileReader(-1, name, MF_READSIZE, 0));

        fileReader->core::Descriptor::operator=(descriptor.release());

        return fileReader.release();
    }

    FileReader* FileReader::open(std::string_view path, int flags) {
        return open(AT_FDCWD, path, flags);
    }

    FileReader* FileReader::open(const std::string& path, const std::function<void(int)>& callback) {
        DescriptorGuard descriptor(core::system::open(path.c_str(), O_RDONLY));
        const int openErrno = descriptor.get() < 0 ? errno : 0;

        errno = openErrno;
        callback(descriptor.get());

        if (descriptor.get() < 0) {
            errno = openErrno;
            return nullptr;
        }

        const std::string name = "FileReader: " + path;
        FileReader* fileReader = create(descriptor.release(), name);
        errno = openErrno;

        return fileReader;
    }

    FileReader* FileReader::open(int directoryFd, std::string_view path, int flags) {
        if (requiresMode(flags)) {
            errno = EINVAL;
            return nullptr;
        }

        const std::string pathname(path);
        DescriptorGuard descriptor(core::system::openat(directoryFd, pathname.c_str(), flags));
        const int openErrno = descriptor.get() < 0 ? errno : 0;

        if (descriptor.get() < 0) {
            errno = openErrno;
            return nullptr;
        }

        const std::string name = "FileReader: " + pathname;
        FileReader* fileReader = create(descriptor.release(), name);
        errno = openErrno;

        return fileReader;
    }

    FileReader* FileReader::adopt(int fd) {
        if (fd < 0) {
            errno = EBADF;
            return nullptr;
        }

        DescriptorGuard descriptor(fd);
        const std::string name = "FileReader: descriptor " + std::to_string(fd);
        FileReader* fileReader = create(descriptor.release(), name);
        errno = 0;

        return fileReader;
    }

    bool FileReader::isOpen() {
        return getFd() >= 0 && !stopping;
    }

    void FileReader::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
        if (running && core::eventLoopState() != core::State::STOPPING) {
            if (!suspended) {
                std::vector<char> puffer(pufferSize);

                const ssize_t ret = core::system::read(getFd(), puffer.data(), puffer.capacity());
                if (ret > 0) {
                    if (this->send(puffer.data(), static_cast<std::size_t>(ret)) < 0) {
                        running = false;

                        this->error(errno);
                    }
                } else {
                    running = false;

                    if (ret == 0) {
                        this->eof();
                    } else {
                        this->error(errno);
                    }
                }

                span();
            }
        } else {
            delete this;
        }
    }

    void FileReader::start() {
        if (!running && isOpen()) {
            running = true;

            if (!suspended) {
                span();
            }
        }
    }

    void FileReader::suspend() {
        if (isOpen()) {
            suspended = true;
        }
    }

    void FileReader::resume() {
        if (running && suspended && isOpen()) {
            suspended = false;
            span();
        }
    }

    void FileReader::stop() {
        if (!stopping) {
            if (running) {
                this->eof();
            }

            running = false;
            stopping = true;
            span();
        }
    }

} // namespace core::file
