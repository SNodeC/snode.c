/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>   // for min
#include <ctime>       // for time
#include <map>         // for map
#include <type_traits> // for __strip_reference_wrapper<>::__type, add_cons...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ReadEventDispatcher.h"

// IWYU pragma: no_include "Reader.h"

#define MAX_READ_INACTIVITY 90

namespace net {

    std::tuple<int, int> ReadEventDispatcher::dispatch(const fd_set& fdSet, int counter, time_t currentTime) {
        time_t nextInactivityTimeout = -1;

        for (const auto& [fd, eventReceivers] : observedEvents) {
            if (FD_ISSET(fd, &fdSet)) {
                counter--;
                eventReceivers.front()->readEvent();
                eventReceivers.front()->lastTriggered = currentTime;
                if (nextInactivityTimeout == -1) {
                    nextInactivityTimeout = MAX_READ_INACTIVITY;
                }
            } else {
                time_t inactivity = currentTime - eventReceivers.front()->lastTriggered;
                if (inactivity >= MAX_READ_INACTIVITY) {
                    eventReceivers.front()->disable();
                } else {
                    if (nextInactivityTimeout == -1) {
                        nextInactivityTimeout = MAX_READ_INACTIVITY - inactivity;
                    } else {
                        nextInactivityTimeout = std::min(MAX_READ_INACTIVITY - inactivity, nextInactivityTimeout);
                    }
                }
            }
        }

        return std::make_tuple(counter, nextInactivityTimeout);
    }

} // namespace net
