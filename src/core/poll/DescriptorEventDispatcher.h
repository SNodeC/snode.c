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

#ifndef CORE_POLL_DESCRIPTOREVENTDISPATCHER_H
#define CORE_POLL_DESCRIPTOREVENTDISPATCHER_H

#include "core/DescriptorEventDispatcher.h" // IWYU pragma: export

namespace core {
    class EventReceiver;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstdint>
#include <list>
#include <map>
#include <sys/epoll.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    class DescriptorEventDispatcher : public core::DescriptorEventDispatcher {
        DescriptorEventDispatcher(const DescriptorEventDispatcher&) = delete;
        DescriptorEventDispatcher& operator=(const DescriptorEventDispatcher&) = delete;

    private:
        class EPollEvents {
        public:
            EPollEvents();

            void add(core::EventReceiver* eventReceiver, uint32_t events);
            void mod(core::EventReceiver* eventReceiver, uint32_t events);
            void del(core::EventReceiver* eventReceiver);

            int getEPFd() const;
            epoll_event* getEvents();
            int getMaxEvents() const;
            void compress();
            void printStats();

        private:
            int epfd;
            std::vector<epoll_event> ePollEvents;
            uint32_t size;
        };

    public:
        explicit DescriptorEventDispatcher(uint32_t events);

        void enable(core::EventReceiver* eventReceiver) override;
        void disable(core::EventReceiver* eventReceiver) override;
        void suspend(core::EventReceiver* eventReceiver) override;
        void resume(core::EventReceiver* eventReceiver) override;

        int getReceiverCount() const;

        int getEPFd() const;

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime) const;

        void observeEnabledEvents();
        void dispatchActiveEvents(const utils::Timeval& currentTime);
        void dispatchImmediateEvents(const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);
        void stop() override;

    private:
        unsigned long getEventCounter() const override;

        class EventReceiverList : public std::list<core::EventReceiver*> {
        public:
            using std::list<core::EventReceiver*>::begin;
            using std::list<core::EventReceiver*>::end;
            using std::list<core::EventReceiver*>::front;

            bool contains(core::EventReceiver* descriptorEventReceiver) const;
        };

        std::map<int, EventReceiverList> enabledEventReceiver;
        std::map<int, EventReceiverList> observedEventReceiver;
        std::map<int, EventReceiverList> disabledEventReceiver;

        EPollEvents ePollEvents;
        uint32_t events;

        unsigned long eventCounter = 0;
    };

} // namespace core::poll

#endif // CORE_POLL_DESCRIPTOREVENTDISPATCHER_H
