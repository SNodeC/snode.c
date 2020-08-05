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

#include <map>     // for map
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "OutOfBandEventDispatcher.h"

// IWYU pragma: no_include "Exception.h"

int OutOfBandEventDispatcher::dispatch(const fd_set& fdSet, int count) {
    if (count > 0) {
        for (auto [fd, eventReceiver] : observedEvents) {
            if (count == 0) {
                break;
            }
            if (FD_ISSET(fd, &fdSet)) {
                count--;
                eventReceiver->outOfBandEvent();
            }
        }
    }

    return count;
}
