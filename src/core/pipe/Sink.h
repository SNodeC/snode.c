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

#ifndef CORE_PIPE_SINK_H
#define CORE_PIPE_SINK_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    class Source;

    class Sink {
    public:
        Sink();

        Sink(Sink&) = delete;
        Sink(Sink&&) = default;

        Sink& operator=(Sink&) = delete;
        Sink& operator=(Sink&&) = default;

        virtual ~Sink();

        void stop();

    private:
        void pipe(Source* source);

        void streamData(const char* junk, std::size_t junkLen);
        void streamEof(const Source* source);
        void streamError(const Source* source, int errnum);

        void disconnect(const Source* source);

        virtual void onSourceConnect(Source* source) = 0;
        virtual void onSourceData(const char* junk, std::size_t junkLen) = 0;
        virtual void onSourceEof() = 0;
        virtual void onSourceError(int errnum) = 0;

        Source* source;

        friend class Source;
    };

} // namespace core::pipe

#endif // CORE_PIPE_SINK_H
