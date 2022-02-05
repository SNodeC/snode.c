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

#ifndef CORE_DESCRIPTOREVENTDISPATCHER_H
#define CORE_DESCRIPTOREVENTDISPATCHER_H

namespace core {
    class Event;
    class DescriptorEventReceiver;
} // namespace core

namespace utils {
    class Timeval;
} // namespace utils

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <deque>
#include <list> // IWYU pragma: export
#include <map>  // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class DescriptorEventDispatcher {
        DescriptorEventDispatcher(const DescriptorEventDispatcher&) = delete;
        DescriptorEventDispatcher& operator=(const DescriptorEventDispatcher&) = delete;

    protected:
        DescriptorEventDispatcher() = default;

    public:
        virtual ~DescriptorEventDispatcher() = default;

        void publish(Event* event);

        void executeEventQueue();

        void enable(DescriptorEventReceiver* eventReceiver);
        void disable(DescriptorEventReceiver* eventReceiver);
        void suspend(DescriptorEventReceiver* eventReceiver);
        void resume(DescriptorEventReceiver* eventReceiver);

        void observeEnabledEvents(const utils::Timeval& currentTime);
        virtual void dispatchActiveEvents(const utils::Timeval& currentTime) = 0;
        void dispatchImmediateEvents(const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);
        virtual void finishTick();

        int getObservedEventReceiverCount() const;
        int getMaxFd() const;

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime) const;

        void stop();

    protected:
        virtual void modAdd(DescriptorEventReceiver* eventReceiver) = 0;
        virtual void modDel(DescriptorEventReceiver* eventReceiver) = 0;
        virtual void modOn(DescriptorEventReceiver* eventReceiver) = 0;
        virtual void modOff(DescriptorEventReceiver* eventReceiver) = 0;

    protected:
        class EventReceiverList : public std::list<core::DescriptorEventReceiver*> {
        public:
            using std::list<core::DescriptorEventReceiver*>::begin;
            using std::list<core::DescriptorEventReceiver*>::end;
            using std::list<core::DescriptorEventReceiver*>::front;

            bool contains(core::DescriptorEventReceiver* descriptorEventReceiver) const;
        };

        std::map<int, EventReceiverList> enabledEventReceiver;
        std::map<int, EventReceiverList> observedEventReceiver;
        std::map<int, EventReceiverList> disabledEventReceiver;
        std::map<int, EventReceiverList> unobservedEventReceiver;

        std::deque<Event*> eventQueue;

        unsigned long eventCounter = 0;
    };

} // namespace core

#endif // CORE_DESCRIPTOREVENTDISPATCHER_H
