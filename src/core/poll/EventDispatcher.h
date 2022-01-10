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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    class EventDispatcher : public core::EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    private:
        class PollFds {
        public:
            PollFds();

            void add(core::EventReceiver* eventReceiver, short events);
            void del(core::EventReceiver* eventReceiver, short events);

            pollfd* getEvents();
            int getMaxEvents() const;
            void compress();
            void printStats();

        private:
            std::vector<pollfd> pollFds;
            uint32_t size;
        };
        /*
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
        */
        PollFds pollFds;
    };

} // namespace core::poll

#endif // CORE_POLL_EVENTDISPATCHER_H
