/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

    bool Source::pipe(Sink* sink) {
        const bool success = sink != nullptr && isOpen();

        if (success) {
            this->sink = sink;
            sink->pipe(this);
        }

        return success;
    }

    bool Source::pipe(const std::shared_ptr<Sink>& sink) {
        return pipe(sink.get());
    }

    void Source::disconnect(const Sink* sink) {
        if (sink == this->sink) {
            this->sink->disconnect(this);
            this->sink = nullptr;

            stop();
        }
    }

    ssize_t Source::send(const char* chunk, std::size_t chunkLen) {
        ssize_t ret = static_cast<ssize_t>(chunkLen);

        if (sink != nullptr) {
            sink->streamData(chunk, chunkLen);
        } else {
            ret = -1;
            errno = EPIPE;
        }

        return ret;
    }

    void Source::eof() {
        if (sink != nullptr) {
            sink->streamEof();
        }
    }

    void Source::error(int errnum) {
        if (sink != nullptr) {
            sink->streamError(errnum);
        }
    }

} // namespace core::pipe
