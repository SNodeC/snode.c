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

#include "core/pipe/Sink.h"

#include "core/pipe/Source.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    Sink::~Sink() {
        if (source != nullptr) {
            source->disconnect(this);
        }
    }

    void Sink::pipe(Source* source) {
        this->source = source;

        onSourceConnect(source);
    }

    bool Sink::isStreaming() {
        return source != nullptr;
    }

    void Sink::stop() {
        if (source != nullptr) {
            source->disconnect(this);
        }
    }

    void Sink::streamData(const char* chunk, std::size_t chunkLen) {
        onSourceData(chunk, chunkLen);
    }

    void Sink::streamEof() {
        onSourceEof();
    }

    void Sink::streamError(int errnum) {
        onSourceError(errnum);
    }

    void Sink::disconnect(const Source* source) {
        if (source == this->source) {
            this->source = nullptr;
        }
    }

} // namespace core::pipe
