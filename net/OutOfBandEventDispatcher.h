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

#ifndef MANAGEDEXCEPTIONS_H
#define MANAGEDEXCEPTIONS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventDispatcher.h"
#include "OutOfBandEventReceiver.h"

// IWYU pragma: no_forward_declare Exception

class OutOfBandEventDispatcher : public EventDispatcher<OutOfBandEventReceiver> {
public:
    using EventDispatcher<OutOfBandEventReceiver>::EventDispatcher;

    int dispatch(const fd_set& fdSet, int count) override;
};

#endif // MANAGEDEXCEPTIONS_H
