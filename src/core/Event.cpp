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

#include "core/Event.h"

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    core::Event::Event(EventReceiver* eventReceiver, const std::string& name)
        : name(name)
        , eventReceiver(eventReceiver)
        , eventMultiplexer(core::EventLoop::instance().getEventMultiplexer()) {
    }

    Event::~Event() {
        if (published) {
            relax();
        }
    }

    void Event::span() {
        if (!published) {
            published = true;
            eventMultiplexer.span(this);
        }
    }

    void Event::relax() {
        if (published) {
            published = false;
            eventMultiplexer.relax(this);
        }
    }

    const std::string& Event::getName() const {
        return name;
    }

    void Event::dispatch(const utils::Timeval& currentTime) {
        published = false;
        eventReceiver->onEvent(currentTime);
    }

    EventReceiver* Event::getEventReceiver() const {
        return eventReceiver;
    }

} // namespace core
