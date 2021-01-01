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

#ifndef WRITESTREAM_H
#define WRITESTREAM_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stream {

    class ReadStream;

    class WriteStream {
    public:
        WriteStream();
        virtual ~WriteStream();

        virtual void pipe(ReadStream& readStream, const char* junk, std::size_t junkLen) = 0;
        virtual void pipeEOF(ReadStream& readStream) = 0;
        virtual void pipeError(ReadStream& readStream, [[maybe_unused]] int errnum) = 0;

        void sourceStream(ReadStream& readStream);
        void unSourceStream(ReadStream& readStream);

    protected:
        std::list<ReadStream*> readStreams;
    };

} // namespace net::stream

#endif // WRITESTREAM_H
