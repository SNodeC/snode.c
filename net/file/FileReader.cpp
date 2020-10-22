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

#include "FileReader.h"
#include "Logger.h"

FileReader::FileReader(int fd, const std::function<void(char* junk, int junkLen)>& junkRead, const std::function<void(int err)>& onError)
    : junkRead(junkRead)
    , onError(onError) {
    open(fd);
    ReadEventReceiver::enable();
}

FileReader::~FileReader() {
    VLOG(1) << "FileReader::~FileReader¨(): " << this;
}

FileReader* FileReader::read(const std::string& path,
                             const std::function<void(char* junk, int junkLen)>& junkRead,
                             const std::function<void(int err)>& onError) {
    FileReader* fileReader = nullptr;

    int fd = ::open(path.c_str(), O_RDONLY);

    if (fd >= 0) {
        fileReader = new FileReader(fd, junkRead, onError);
    } else {
        onError(errno);
    }

    return fileReader;
}

void FileReader::readEvent() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    static char junk[MFREADSIZE];

    int ret = ::read(getFd(), junk, MFREADSIZE);

    if (ret > 0) {
        junkRead(junk, ret);
    } else {
        onError(ret == 0 ? 0 : errno);
    }
}

void FileReader::unobserved() {
    delete this;
}
