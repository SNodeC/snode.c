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

#ifndef SINK_H
#define SINK_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stream {

    class Source;

    class Sink {
    public:
        Sink();
        virtual ~Sink();

        virtual void receive(Source& source, const char* junk, std::size_t junkLen) = 0;
        virtual void eof(Source& source) = 0;
        virtual void error(Source& source, [[maybe_unused]] int errnum) = 0;

        void connect(Source& source);
        void disconnect(Source& source);

    protected:
        std::list<Source*> sources;
    };

} // namespace net::stream

#endif // SINK_H
