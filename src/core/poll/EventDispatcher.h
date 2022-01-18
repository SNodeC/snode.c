/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef CORE_POLL_EVENTDISPATCHER_H
#define CORE_POLL_EVENTDISPATCHER_H

#include "core/EventDispatcher.h"

namespace core {
    class EventReceiver;
}

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // for uint32_t
#include <poll.h>
#include <unordered_map>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    struct PollFdIndex {
        using pollfds_size_type = std::vector<pollfd>::size_type;

        pollfds_size_type index; // cppcheck-suppress unusedStructMember
        short events;            // cppcheck-suppress unusedStructMember
    };

    class PollFds {
    public:
        using pollfds_size_type = std::vector<pollfd>::size_type;
        explicit PollFds();

        void add(core::EventReceiver* eventReceiver, short event);
        void del(core::EventReceiver* eventReceiver, short event);

        void modOn(core::EventReceiver* eventReceiver, short event);
        void modOff(core::EventReceiver* eventReceiver, short event);

        void compress();

        pollfd* getEvents();

        std::unordered_map<int, PollFdIndex>& getPollFdIndices();

        nfds_t getInterestCount() const;

    private:
        std::vector<pollfd> pollfds;
        std::unordered_map<int, PollFdIndex> pollFdIndices;

        uint32_t interestCount;
    };

    class EventDispatcher : public core::EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    public:
        EventDispatcher();
        ~EventDispatcher() = default;

    private:
        int multiplex(utils::Timeval& tickTimeOut) override;
        void dispatchActiveEvents(int count, const utils::Timeval& currentTime) override;

    private:
        PollFds pollFds;
    };

} // namespace core::poll

#endif // CORE_POLL_EVENTDISPATCHER_H
