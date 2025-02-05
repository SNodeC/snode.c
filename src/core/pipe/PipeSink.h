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

#ifndef CORE_PIPE_PIPESINK_H
#define CORE_PIPE_PIPESINK_H

#include "core/eventreceiver/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    class PipeSink : public core::eventreceiver::ReadEventReceiver {
    public:
        PipeSink(const PipeSink&) = delete;

        PipeSink& operator=(const PipeSink&) = delete;

        explicit PipeSink(int fd);
        ~PipeSink() override;

        void setOnData(const std::function<void(const char*, std::size_t)>& onData);
        void setOnEof(const std::function<void()>& onEof);
        void setOnError(const std::function<void(int)>& onError);

    private:
        void readEvent() override;
        void unobservedEvent() override;

        std::function<void(const char*, std::size_t)> onData;
        std::function<void()> onEof;
        std::function<void(int errnum)> onError;
    };

} // namespace core::pipe

#endif // CORE_PIPE_PIPESINK_H
