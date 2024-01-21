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

// constexpr int MF_READSIZE = 16384;

namespace core::file {

    FileReader::FileReader(int fd, core::pipe::Sink& sink, const std::string& name)
        : core::Descriptor(fd)
        , EventReceiver(name) {
        Source::connect(sink);
    }

    FileReader::FileReader(int fd, core::pipe::Sink& sink, const std::string& name, std::size_t pufferSize)
        : core::Descriptor(fd)
        , EventReceiver(name)
        , pufferSize(pufferSize) {
        Source::connect(sink);

        span();
    }

    FileReader* FileReader::open(const std::string& path, core::pipe::Sink& sink, const std::function<void(int err)>& onStatus) {
        errno = 0;

        FileReader* fileReader = nullptr;

        const int fd = core::system::open(path.c_str(), O_RDONLY);

        if (fd >= 0) {
            fileReader = new FileReader(fd, sink, "FileReader: " + path);
        }

        onStatus(errno);

        return fileReader;
    }

    ssize_t FileReader::read(std::size_t pufferSize) {
        std::vector<char> puffer;
        puffer.reserve(pufferSize);

        const ssize_t ret = core::system::read(getFd(), puffer.data(), puffer.capacity());

        if (ret > 0) {
            if (send(puffer.data(), static_cast<std::size_t>(ret)) < 0) {
                this->error(errno);
            }
        } else if (ret == 0) {
            this->eof();
        } else {
            this->error(errno);
        }

        return ret;
    }

    void FileReader::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
        if (core::eventLoopState() != core::State::STOPPING) {
            read(pufferSize);
        }
    }

} // namespace core::file
