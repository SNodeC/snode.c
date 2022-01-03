/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "N_EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    const utils::Timeval N_EventReceiver::Timeout::DEFAULT = {-1, 0};
    const utils::Timeval N_EventReceiver::Timeout::DISABLE = {LONG_MAX, 0};

    N_EventReceiver::N_EventReceiver(EventDispatchers& eventDispatcher, const utils::Timeval& timeout)
        : eventDispatcher(eventDispatcher)
        , maxInactivity(timeout)
        , initialTimeout(timeout)
        , lastTriggered({0, 0}) {
    }

    void N_EventReceiver::setTimeout(const utils::Timeval& timeout) {
        if (timeout == Timeout::DEFAULT) {
            this->maxInactivity = initialTimeout;
        } else {
            this->maxInactivity = timeout;
        }

        triggered();
    }

    utils::Timeval N_EventReceiver::getTimeout(const utils::Timeval& currentTime) const {
        return std::max(maxInactivity - (currentTime - lastTriggered), utils::Timeval());
    }

    void N_EventReceiver::triggered() {
        lastTriggered = utils::Timeval::currentTime();
    }

    void N_EventReceiver::activate() {
        state = State::ACTIVE;
    }

    void N_EventReceiver::inactivate() {
        state = State::INACTIVE;
    }

    void N_EventReceiver::stop() {
        state = State::STOPPED;
    }

    void N_EventReceiver::terminate() {
        if (isActive()) {
            inactivate();
        }
    }

    bool N_EventReceiver::isNew() const {
        return state == State::NEW;
    }

    bool N_EventReceiver::isActive() const {
        return state == State::ACTIVE;
    }

    bool N_EventReceiver::isInactive() const {
        return state == State::INACTIVE;
    }

    bool N_EventReceiver::isStopped() const {
        return state == State::STOPPED;
    }

} // namespace core
