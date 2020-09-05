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

#include "WriteEventDispatcher.h"

// IWYU pragma: no_include "Writer.h"

namespace net {

    std::tuple<int, int> WriteEventDispatcher::dispatch(const fd_set& fdSet, int counter, time_t currentTime) {
        time_t nextInactivityTimeout = -1;

        for (const auto& [fd, eventReceivers] : observedEvents) {
            if (FD_ISSET(fd, &fdSet)) {
                counter--;
                eventReceivers.front()->writeEvent();
                eventReceivers.front()->lastTriggered = currentTime;
                if (nextInactivityTimeout == -1 && maxInactivity > 0) {
                    nextInactivityTimeout = maxInactivity;
                }
            } else {
                if (maxInactivity > 0) {
                    time_t inactivity = currentTime - eventReceivers.front()->lastTriggered;
                    if (inactivity >= maxInactivity) {
                        eventReceivers.front()->disable();
                    } else {
                        if (nextInactivityTimeout == -1) {
                            nextInactivityTimeout = maxInactivity - inactivity;
                        } else {
                            nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                        }
                    }
                }
            }
        }

        return std::make_tuple(counter, nextInactivityTimeout);
    }

} // namespace net
