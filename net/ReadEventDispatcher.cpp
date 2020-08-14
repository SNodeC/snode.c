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

#include <list>
#include <map> // for map

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ReadEventDispatcher.h"

// IWYU pragma: no_include "Reader.h"

int ReadEventDispatcher::dispatch(const fd_set& fdSet, int counter) {
    if (counter > 0) {
        for (auto [fd, eventReceivers] : observedEvents) {
            if (counter == 0) {
                break;
            }
            if (FD_ISSET(fd, &fdSet)) {
                counter--;
                eventReceivers.front()->readEvent();
            }
        }
    }

    return counter;
}
