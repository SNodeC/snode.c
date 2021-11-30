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

#ifndef NET_EVENTDISPATCHER_H
#define NET_EVENTDISPATCHER_H

#include "core/system/select.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <sys/time.h> // for timeval

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventReceiver;

    class EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    private:
        class FdSet {
        public:
            FdSet();

            void set(int fd);
            void clr(int fd);
            int isSet(int fd) const;
            void zero();
            fd_set& get();

        protected:
            fd_set registered;
            fd_set active;
        };

    public:
        EventDispatcher() = default;

        void enable(EventReceiver* eventReceiver, int fd);
        void disable(EventReceiver* eventReceiver, int fd);
        void suspend(EventReceiver* eventReceiver, int fd);
        void resume(EventReceiver* eventReceiver, int fd);

        unsigned long getEventCounter() const;

    private:
        class DescriptorEventReceiverList : public std::list<EventReceiver*> {
        public:
            using std::list<EventReceiver*>::begin;
            using std::list<EventReceiver*>::end;
            using std::list<EventReceiver*>::front;

            bool contains(EventReceiver* descriptorEventReceiver) const;
        };

        int getMaxFd() const;
        fd_set& getFdSet();

        struct timeval getNextTimeout(struct timeval currentTime) const;

        void observeEnabledEvents();
        void dispatchActiveEvents(struct timeval currentTime);
        void unobserveDisabledEvents();
        void releaseUnobservedEvents();

        void terminateObservedEvents();

        std::map<int, DescriptorEventReceiverList> enabledEventReceiver;
        std::map<int, DescriptorEventReceiverList> observedEventReceiver;
        std::map<int, DescriptorEventReceiverList> disabledEventReceiver;
        DescriptorEventReceiverList unobservedEventReceiver;

        FdSet fdSet;

        unsigned long eventCounter = 0;

        friend class EventLoop;
    }; // namespace net

} // namespace core

#endif // NET_EVENTDISPATCHER_H
