/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/pipe/PipeSink.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <cerrno>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_READ_JUNKSIZE
#define MAX_READ_JUNKSIZE 16384
#endif

namespace core::pipe {

    PipeSink::PipeSink(int fd)
        : Descriptor(fd)
        , core::eventreceiver::ReadEventReceiver("PipeSink fd = " + std::to_string(fd)) {
        ReadEventReceiver::enable(fd);
    }

    void PipeSink::readEvent() {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static char junk[MAX_READ_JUNKSIZE];

        ssize_t ret = core::system::read(Descriptor::getFd(), junk, MAX_READ_JUNKSIZE);

        if (ret > 0) {
            if (onData) {
                onData(junk, static_cast<std::size_t>(ret));
            }
        } else {
            ReadEventReceiver::disable();

            if (ret == 0) {
                if (onEof) {
                    onEof();
                }
            } else {
                if (onError) {
                    onError(errno);
                }
            }
        }
    }

    void PipeSink::setOnData(const std::function<void(const char* junk, std::size_t junkLen)>& onData) {
        this->onData = onData;
    }

    void PipeSink::setOnEof(const std::function<void()>& onEof) {
        this->onEof = onEof;
    }

    void PipeSink::setOnError(const std::function<void(int errnum)>& onError) {
        this->onError = onError;
    }

    void PipeSink::terminate() {
        ReadEventReceiver::terminate();
    }

    void PipeSink::unobservedEvent() {
        delete this;
    }

} // namespace core::pipe
