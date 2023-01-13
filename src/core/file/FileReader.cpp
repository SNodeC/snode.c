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

#define MFREADSIZE 16384

#include "core/file/FileReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"
#include "utils/Timeval.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::file {

    FileReader::FileReader(int fd, core::pipe::Sink& sink, const std::string& name)
        : EventReceiver(name) {
        open(fd);
        Source::connect(sink);

        span();
    }

    FileReader* FileReader::connect(const std::string& path, core::pipe::Sink& writeStream, const std::function<void(int err)>& onError) {
        errno = 0;

        FileReader* fileReader = nullptr;

        int fd = core::system::open(path.c_str(), O_RDONLY);

        if (fd >= 0) {
            fileReader = new FileReader(fd, writeStream, "FileReader: " + path);
        }

        onError(errno);

        return fileReader;
    }

    void FileReader::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
        if (!suspended) {
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
            static char junk[MFREADSIZE];

            ssize_t ret = core::system::read(getFd(), junk, MFREADSIZE);

            if (ret > 0) {
                if (send(junk, static_cast<std::size_t>(ret)) >= 0) {
                    span();
                } else {
                    delete this;
                }
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

    void FileReader::suspend() {
        suspended = true;
    }

    void FileReader::resume() {
        suspended = false;
        span();
    }

    bool FileReader::isSuspended() {
        return suspended;
    }

} // namespace core::file
