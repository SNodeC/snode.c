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

#include "net/stream/Source.h"

#include "net/stream/Sink.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stream {

    Source::Source()
        : sink(nullptr) {
    }

    Source::~Source() {
        if (sink != nullptr) {
            sink->disconnect(*this);
        }
    }

    void Source::connect(Sink& sink) {
        this->sink = &sink;
        if (this->sink != nullptr) {
            sink.connect(*this);
        }
    }

    void Source::disconnect(Sink& sink) {
        if (&sink == this->sink) {
            this->sink = nullptr;
        }
    }

    void Source::send(const char* junk, std::size_t junkLen) {
        if (this->sink != nullptr) {
            sink->receive(junk, junkLen);
        }
    }

    void Source::error(int errnum) {
        if (this->sink != nullptr) {
            sink->error(errnum);
        }
    }

    void Source::eof() {
        if (this->sink != nullptr) {
            sink->eof();
        }
    }

} // namespace net::stream
