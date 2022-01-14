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

#ifndef CORE_POLL_EVENTDISPATCHER_H
#define CORE_POLL_EVENTDISPATCHER_H

#include "core/EventDispatcher.h"
#include "core/poll/DescriptorEventDispatcher.h"
#include "core/poll/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/poll.h>
#include <unordered_map>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    class PollFds {
    public:
        using pollfds_size_type = std::vector<pollfd>::size_type;
        PollFds();

        void add(core::EventReceiver* eventReceiver, short event);
        void del(core::EventReceiver* eventReceiver, short event);

    public:
        void modOn(core::EventReceiver* eventReceiver, short event);
        void modOff(core::EventReceiver* eventReceiver, short event);

        void dispatch(const utils::Timeval& currentTime);

        pollfd* getEvents();
        nfds_t getMaxEvents() const;
        void compress();
        void printStats(const std::string& what);

    private:
        struct PollEvent {
            explicit PollEvent(pollfds_size_type fds);

            pollfds_size_type fds;
            std::unordered_map<short, core::EventReceiver*> eventReceivers;
        };

        std::vector<pollfd> pollFds;

        // Use an unordered_map: It is a hash_map
        std::unordered_map<int, PollEvent, std::hash<int>> pollEvents; // map fd -> index into pollFds;
        uint32_t interestCount;
    };

    class EventDispatcher : public core::EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    public:
        EventDispatcher();
        ~EventDispatcher() = default;

        core::DescriptorEventDispatcher& getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) override;
        core::TimerEventDispatcher& getTimerEventDispatcher() override;

        TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped) override;
        void stop() override;

    private:
        int getReceiverCount();
        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void observeEnabledEvents();
        void dispatchActiveEvents(int count, const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

        core::poll::DescriptorEventDispatcher eventDispatcher[3];
        core::poll::TimerEventDispatcher timerEventDispatcher;

        PollFds pollFds;
    };

} // namespace core::poll

#endif // CORE_POLL_EVENTDISPATCHER_H
