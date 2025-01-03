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

#ifndef CORE_PIPE_SOURCE_H
#define CORE_PIPE_SOURCE_H

namespace core::pipe {
    class Sink;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <memory>
#include <sys/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    class Source {
    public:
        Source() = default;

        Source(Source&) = delete;
        Source(Source&&) noexcept = default;

        Source& operator=(Source&) = delete;
        Source& operator=(Source&&) noexcept = default;

        virtual ~Source();

        virtual bool isOpen() = 0;

        void pipe(Sink* sink, const std::function<void(int)>& callback);
        void pipe(const std::shared_ptr<Sink>& sink, const std::function<void(int)>& callback);

        virtual void start() = 0;
        virtual void suspend() = 0;
        virtual void resume() = 0;
        virtual void stop() = 0;

    protected:
        ssize_t send(const char* chunk, std::size_t chunkLen);
        void eof();
        void error(int errnum);

    private:
        void disconnect(const Sink* sink);

        Sink* sink = nullptr;

        friend class Sink;
    };

} // namespace core::pipe

#endif // CORE_PIPE_SOURCE_H
