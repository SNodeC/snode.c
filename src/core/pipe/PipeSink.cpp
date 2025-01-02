/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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
#include "utils/Timeval.h"

#include <cerrno>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_READ_JUNKSIZE
#define MAX_READ_JUNKSIZE 16384
#endif

namespace core::pipe {

    PipeSink::PipeSink(int fd)
        : core::eventreceiver::ReadEventReceiver("PipeSink fd = " + std::to_string(fd), 60) {
        if (!ReadEventReceiver::enable(fd)) {
            delete this;
        }
    }

    PipeSink::~PipeSink() {
        close(getRegisteredFd());
    }

    void PipeSink::readEvent() {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static char chunk[MAX_READ_JUNKSIZE];

        const ssize_t ret = core::system::read(getRegisteredFd(), chunk, MAX_READ_JUNKSIZE);

        if (ret > 0) {
            if (onData) {
                onData(chunk, static_cast<std::size_t>(ret));
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

    void PipeSink::setOnData(const std::function<void(const char*, std::size_t)>& onData) {
        this->onData = onData;
    }

    void PipeSink::setOnEof(const std::function<void()>& onEof) {
        this->onEof = onEof;
    }

    void PipeSink::setOnError(const std::function<void(int)>& onError) {
        this->onError = onError;
    }

    void PipeSink::unobservedEvent() {
        delete this;
    }

} // namespace core::pipe
