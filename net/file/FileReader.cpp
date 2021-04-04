/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MFREADSIZE 16384

#include "net/file/FileReader.h"

FileReader::FileReader(int fd, net::stream::Sink& sink) {
    attach(fd);
    ReadEventReceiver::enable(fd);
    Source::connect(sink);
}

FileReader* FileReader::connect(const std::string& path, net::stream::Sink& writeStream, const std::function<void(int err)>& onError) {
    FileReader* fileReader = nullptr;

    int fd = ::open(path.c_str(), O_RDONLY);

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

    ssize_t ret = ::read(getFd(), junk, MFREADSIZE);

    if (ret > 0) {
        this->send(junk, ret);
    } else {
        ReadEventReceiver::disable();
        if (ret == 0) {
            this->eof();
        } else {
            this->error(errno);
        }
    }
}

void FileReader::unobserved() {
    delete this;
}
