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

#include "core/pipe/Source.h"

#include "core/pipe/Sink.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    Source::~Source() {
        if (sink != nullptr) {
            sink->disconnect(this);
        }
    }

    void Source::pipe(Sink* sink, const std::function<void(int)>& callback) {
        if (sink != nullptr) {
            if (isOpen()) {
                callback(0);

                this->sink = sink;
                sink->pipe(this);
            } else {
                callback(errno);
                stop();
            }
        } else {
            callback(EFAULT);
            stop();
        }
    }

    void Source::pipe(std::shared_ptr<Sink> sink, const std::function<void(int)>& callback) {
        pipe(sink.get(), callback);
    }

    void Source::disconnect(const Sink* sink) {
        if (sink == this->sink) {
            this->sink = nullptr;

            stop();
        }
    }

    ssize_t Source::send(const char* chunk, std::size_t chunkLen) {
        ssize_t ret = static_cast<ssize_t>(chunkLen);

        if (this->sink != nullptr) {
            sink->streamData(chunk, chunkLen);
        } else {
            ret = -1;
            errno = EPIPE;
        }

        return ret;
    }

    void Source::error(int errnum) {
        if (this->sink != nullptr) {
            sink->streamError(errnum);
        }
    }

    void Source::eof() {
        if (this->sink != nullptr) {
            sink->streamEof();
        }
    }

} // namespace core::pipe
