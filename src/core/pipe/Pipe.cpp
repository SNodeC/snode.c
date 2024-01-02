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

#include "core/pipe/Pipe.h"

#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    Pipe::Pipe(const std::function<void(PipeSource& pipeSource, PipeSink& pipsSink)>& onSuccess,
               const std::function<void(int err)>& onError) {
        const int ret = core::system::pipe2(pipeFd, O_NONBLOCK);

        if (ret == 0) {
            onSuccess(*(new PipeSource(pipeFd[1])), *(new PipeSink(pipeFd[0])));
        } else {
            onError(errno);
        }
    }

} // namespace core::pipe
