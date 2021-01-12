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

#include "Source.h"

#include "Sink.h"

namespace net::stream {

    Source::Source() {
    }

    Source::~Source() {
        for (Sink* sink : sinks) {
            sink->disconnect(*this);
        }
    }

    void Source::connect(Sink& sink) {
        sinks.push_back(&sink);
        sink.connect(*this);
    }

    void Source::disconnect(Sink& sink) {
        if (dispatching) {
            disconnectedSinks.push_back(&sink);
        } else {
            sinks.remove(&sink);
        }
    }

    void Source::send(const char* junk, std::size_t junkLen) {
        dispatching = true;
        for (Sink* sink : sinks) {
            sink->receive(*this, junk, junkLen);
        }
        dispatching = false;
        for (Sink* sink : disconnectedSinks) {
            sinks.remove(sink);
        }
        disconnectedSinks.clear();
    }

    void Source::error(int errnum) {
        dispatching = true;
        for (Sink* sink : sinks) {
            sink->error(*this, errnum);
        }
        dispatching = false;
        for (Sink* sink : disconnectedSinks) {
            sinks.remove(sink);
        }
        disconnectedSinks.clear();
    }

    void Source::eof() {
        dispatching = true;
        for (Sink* sink : sinks) {
            sink->eof(*this);
        }
        dispatching = false;
        for (Sink* sink : disconnectedSinks) {
            sinks.remove(sink);
        }
        disconnectedSinks.clear();
    }

} // namespace net::stream
