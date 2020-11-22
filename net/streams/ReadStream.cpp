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

#include "ReadStream.h"

#include "WriteStream.h"

namespace net::stream {

    ReadStream::ReadStream() {
    }

    ReadStream::~ReadStream() {
        for (WriteStream* writeStream : writeStreams) {
            writeStream->unSourceStream(*this);
        }
    }

    void ReadStream::pipe(WriteStream& writeStream) {
        writeStreams.push_back(&writeStream);
        writeStream.sourceStream(*this);
    }

    void ReadStream::unPipe(WriteStream& writeStream) {
        if (dispatching) {
            unPipedStreams.push_back(&writeStream);
        } else {
            writeStreams.remove(&writeStream);
        }
    }

    void ReadStream::dispatch(const char* junk, size_t junkLen) {
        dispatching = true;
        for (WriteStream* writeStream : writeStreams) {
            writeStream->pipe(*this, junk, junkLen);
        }
        dispatching = false;
        if (unPipedStreams.size() > 0) {
            for (WriteStream* writeStream : unPipedStreams) {
                writeStreams.remove(writeStream);
            }
            unPipedStreams.clear();
        }
    }

    void ReadStream::dispatchError(int errnum) {
        dispatching = true;
        for (WriteStream* writeStream : writeStreams) {
            writeStream->pipeError(*this, errnum);
        }
        dispatching = false;
        if (unPipedStreams.size() > 0) {
            for (WriteStream* writeStream : unPipedStreams) {
                writeStreams.remove(writeStream);
            }
            unPipedStreams.clear();
        }
    }

    void ReadStream::dispatchEOF() {
        dispatching = true;
        for (WriteStream* writeStream : writeStreams) {
            writeStream->pipeEOF(*this);
        }
        dispatching = false;
        if (unPipedStreams.size() > 0) {
            for (WriteStream* writeStream : unPipedStreams) {
                writeStreams.remove(writeStream);
            }
            unPipedStreams.clear();
        }
    }

} // namespace net::stream
