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
#include <fcntl.h>  /* Obtain O_* constant definitions */
#include <unistd.h> // for pipe2

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "net/stream/Pipe.h"
#include "net/stream/PipeSink.h"   // for PipeSink
#include "net/stream/PipeSource.h" // for PipeSource

namespace net::stream {

    Pipe::Pipe(const std::function<void(PipeSource& pipeSource, PipeSink& pipsSink)>& onSuccess,
               const std::function<void(int err)>& onError) {
        int ret = pipe2(pipeFd, O_NONBLOCK);

        if (ret == 0) {
            onSuccess(*(new PipeSource(pipeFd[1])), *(new PipeSink(pipeFd[0])));
        } else {
            onError(errno);
        }
    }

} // namespace net::stream
