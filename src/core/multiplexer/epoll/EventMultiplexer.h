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

#ifndef CORE_EPOLL_EVENTMULTIPLEXER_H
#define CORE_EPOLL_EVENTMULTIPLEXER_H

#include "core/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/epoll.h"

namespace utils {
    class Timeval;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    class EventMultiplexer : public core::EventMultiplexer {
    public:
        EventMultiplexer();
        ~EventMultiplexer() override = default;

    private:
        int monitorDescriptors(utils::Timeval& tickTimeout, const sigset_t& sigMask) override;
        void spanActiveEvents(int activeDescriptorCount) override;

        int epfd;

        int epfds[3];
        epoll_event ePollEvents[3]{};
    };

} // namespace core::epoll

#endif // CORE_EPOLL_EVENTMULTIPLEXER_H
