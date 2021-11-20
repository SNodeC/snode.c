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

#ifndef STREAM_PIPESINK_H
#define STREAM_PIPESINK_H

#include "core/Descriptor.h"
#include "core/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::pipe {

    class PipeSink
        : public Descriptor
        , public ReadEventReceiver {
        PipeSink(const PipeSink&) = delete;
        PipeSink& operator=(const PipeSink&) = delete;

    public:
        PipeSink(int fd);

        void setOnData(const std::function<void(const char* junk, std::size_t junkLen)>& onData);
        void setOnEof(const std::function<void()>& onEof);
        void setOnError(const std::function<void(int errnum)>& onError);

    protected:
        void readEvent() override;
        void unobservedEvent() override;

        std::function<void(const char* junk, std::size_t junkLen)> onData;
        std::function<void()> onEof;
        std::function<void(int errnum)> onError;
    };

} // namespace net::pipe

#endif // STREAM_PIPESINK_H
