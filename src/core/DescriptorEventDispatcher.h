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
    class EventReceiver;
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

        void enable(EventReceiver* eventReceiver);
        void disable(EventReceiver* eventReceiver);
        void suspend(EventReceiver* eventReceiver);
        void resume(EventReceiver* eventReceiver);

        void observeEnabledEvents();
        virtual void dispatchActiveEvents(const utils::Timeval& currentTime) = 0;
        void dispatchImmediateEvents(const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);
        virtual void finishTick();

        int getObservedEventReceiverCount() const;
        int getMaxFd() const;

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime) const;

        void stop();

    protected:
        virtual void modAdd(EventReceiver* eventReceiver) = 0;
        virtual void modDel(EventReceiver* eventReceiver) = 0;
        virtual void modOn(EventReceiver* eventReceiver) = 0;
        virtual void modOff(EventReceiver* eventReceiver) = 0;

    protected:
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
        std::map<int, EventReceiverList> unobservedEventReceiver;

        std::deque<Event*> eventQueue;

        unsigned long eventCounter = 0;
    };

} // namespace core

#endif // CORE_DESCRIPTOREVENTDISPATCHER_H
