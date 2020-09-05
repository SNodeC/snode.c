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

#ifndef READEVENTDISPATCHER_H
#define READEVENTDISPATCHER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h>
#include <tuple> // for tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventDispatcher.h"
#include "ReadEventReceiver.h"

// IWYU pragma: no_forward_declare net::ReadEventReceiver

namespace net {

    class ReadEventDispatcher : public EventDispatcher<ReadEventReceiver> {
    public:
        using EventDispatcher<ReadEventReceiver>::EventDispatcher;

        std::tuple<int, int> dispatch(const fd_set& fdSet, int count) override;
    };

} // namespace net

#endif // READEVENTDISPATCHER_H
