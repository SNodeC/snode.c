/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "core/file/FileReader.h"

#include "core/State.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <cerrno>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

constexpr int MF_READSIZE = 16384;

namespace core::file {

    FileReader::FileReader(int fd, const std::string& name, std::size_t pufferSize, int openErrno)
        : core::Descriptor(fd)
        , EventReceiver(name)
        , pufferSize(pufferSize)
        , openErrno(openErrno) {
    }

    FileReader* FileReader::open(const std::string& path) {
        errno = 0;

        const int fd = core::system::open(path.c_str(), O_RDONLY);

        return new FileReader(fd, "FileReader: " + path, MF_READSIZE, fd < 0 ? errno : 0);
    }

    bool FileReader::isOpen() {
        return getFd() >= 0;
    }

    void FileReader::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
        if (core::eventLoopState() != core::State::STOPPING) {
            if (!suspended) {
                std::vector<char> puffer(pufferSize);

                const ssize_t ret = core::system::read(getFd(), puffer.data(), puffer.capacity());
                if (ret > 0) {
                    if (this->send(puffer.data(), static_cast<std::size_t>(ret)) < 0) {
                        this->error(errno);
                    }
                    span();
                } else {
                    if (ret == 0) {
                        this->eof();
                    } else {
                        this->error(errno);
                    }

                    delete this;
                }
            }
        }
    }

    void FileReader::start() {
        span();
    }

    void FileReader::suspend() {
        suspended = true;
    }

    void FileReader::resume() {
        suspended = false;

        span();
    }

    void FileReader::stop() {
        suspend();

        delete this;
    }

} // namespace core::file
