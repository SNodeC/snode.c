/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <ctime>
#include <list>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "EventReceiver.h"
#include "FdSet.h"
#include "Logger.h"
#include "Timeval.h"

namespace net {

    class EventLoop;

    template <typename EventReceiverT>
    class EventDispatcher {
    public:
        using EventReceiver = EventReceiverT;

    private:
        class EventReceiverList : public std::list<EventReceiver*> {
        public:
            using std::list<EventReceiver*>::begin;
            using std::list<EventReceiver*>::end;
            using std::list<EventReceiver*>::front;

            bool contains(EventReceiver* eventReceiver);
        };

    public:
        explicit EventDispatcher(FdSet& fdSet, long maxInactivity); // NOLINT(google-runtime-references)
        EventDispatcher(const EventDispatcher&) = delete;

        EventDispatcher& operator=(const EventDispatcher&) = delete;

        void enable(EventReceiver* eventReceiver, int fd);
        void disable(EventReceiver* eventReceiver, int fd);
        void suspend(EventReceiver* eventReceiver, int fd);
        void resume(EventReceiver* eventReceiver, int fd);

        unsigned long getEventCounter() const;
        long getTimeout() const;

    private:
        int getMaxFd() const;

        struct timeval observeEnabledEvents();
        struct timeval dispatchActiveEvents(struct timeval currentTime);
        void unobserveDisabledEvents();
        void releaseUnobservedEvents();
        void disableObservedEvents();

        std::map<int, EventReceiverList> enabledEventReceiver;
        std::map<int, EventReceiverList> observedEventReceiver;
        std::map<int, EventReceiverList> disabledEventReceiver;
        EventReceiverList unobservedEventReceiver;

        FdSet& fdSet;

        long maxInactivity;

        unsigned long eventCounter = 0;

        friend class EventLoop;
    }; // namespace net

} // namespace net

#endif // EVENTDISPATCHER_H
