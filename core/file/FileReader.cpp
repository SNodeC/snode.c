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

#define MFREADSIZE 16384

#include "core/file/FileReader.h"

#include "core/Descriptor.h" // for Descriptor
#include "core/system/unistd.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

FileReader::FileReader(int fd, core::pipe::Sink& sink)
    : Descriptor(fd) {
    ReadEventReceiver::enable(fd);
    Source::connect(sink);
}

FileReader* FileReader::connect(const std::string& path, core::pipe::Sink& writeStream, const std::function<void(int err)>& onError) {
    FileReader* fileReader = nullptr;

    int fd = core::system::open(path.c_str(), O_RDONLY);

    if (fd >= 0) {
        fileReader = new FileReader(fd, writeStream);
    } else {
        onError(errno);
    }

    return fileReader;
}

void FileReader::readEvent() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    static char junk[MFREADSIZE];

    ssize_t ret = core::system::read(getFd(), junk, MFREADSIZE);

    if (ret > 0) {
        this->send(junk, static_cast<std::size_t>(ret));
    } else {
        ReadEventReceiver::disable();
        if (ret == 0) {
            this->eof();
        } else {
            this->error(errno);
        }
    }
}

void FileReader::sinkDisconnected() {
    ReadEventReceiver::disable();
}

void FileReader::unobservedEvent() {
    delete this;
}
