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

#include "core/EventMultiplexer.h"

namespace core {
    class DescriptorEventReceiver;
}

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/poll.h" // IWYU pragma: export

#include <unordered_map>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    class PollFds {
    public:
        struct PollFdIndex {
            using pollfds_size_type = std::vector<pollfd>::size_type;

            pollfds_size_type index; // cppcheck-suppress unusedStructMember
            short events;            // cppcheck-suppress unusedStructMember
        };

        using pollfds_size_type = std::vector<pollfd>::size_type;
        explicit PollFds();

        void muxAdd(core::DescriptorEventReceiver* eventReceiver, short event);
        void muxDel(int fd, short event);
        void muxOn(core::DescriptorEventReceiver* eventReceiver, short event);
        void muxOff(int fd, short event);

        pollfd* getEvents();

        const std::unordered_map<int, PollFdIndex>& getPollFdIndices() const;

        nfds_t getCurrentIndex() const;

    private:
        nfds_t interestCount = 0;
        nfds_t nextIndex = 0;
        void compress();

        std::vector<pollfd> pollfds;
        std::unordered_map<int, PollFdIndex> pollFdIndices;
    };

    class EventMultiplexer : public core::EventMultiplexer {
        EventMultiplexer(const EventMultiplexer&) = delete;
        EventMultiplexer& operator=(const EventMultiplexer&) = delete;

    public:
        EventMultiplexer();
        ~EventMultiplexer() override = default;

    private:
        int multiplex(utils::Timeval& tickTimeOut) override;
        void dispatchActiveEvents(int count) override;

    private:
        PollFds pollFds;
    };

} // namespace core::poll

#endif // CORE_POLL_EVENTDISPATCHER_H
