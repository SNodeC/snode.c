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

#include "core/system/select.h" // IWYU pragma: keep
#include "utils/Timeval.h"

namespace core {
    class EventReceiver;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
//#include <sys/time.h> // IWYU pragma: keep

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    public:
        EventDispatcher();
        ~EventDispatcher();

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

        class DescriptorEventReceiverList : public std::list<EventReceiver*> {
        public:
            using std::list<EventReceiver*>::begin;
            using std::list<EventReceiver*>::end;
            using std::list<EventReceiver*>::front;

            bool contains(EventReceiver* descriptorEventReceiver) const;
        };

    public:
        void enable(EventReceiver* eventReceiver, int fd);
        void disable(EventReceiver* eventReceiver, int fd);
        void suspend(EventReceiver* eventReceiver, int fd);
        void resume(EventReceiver* eventReceiver, int fd);

        unsigned long getEventCounter() const;

        fd_set& getFdSet();
        static int getMaxFd();

        static utils::Timeval getNextTimeout();

        static void observeEnabledEvents();
        static void dispatchActiveEvents();
        static void unobserveDisabledEvents();
        static void terminateObservedEvents();

    private:
        int _getMaxFd() const;

        utils::Timeval _getNextTimeout(const utils::Timeval& currentTime) const;

        void _observeEnabledEvents();
        void _dispatchActiveEvents(const utils::Timeval& currentTime);
        void _unobserveDisabledEvents();
        void _terminateObservedEvents();

        static std::list<EventDispatcher*> eventDispatchers;

        std::map<int, DescriptorEventReceiverList> enabledEventReceiver;
        std::map<int, DescriptorEventReceiverList> observedEventReceiver;
        std::map<int, DescriptorEventReceiverList> disabledEventReceiver;

        FdSet fdSet;

        unsigned long eventCounter = 0;
    };

} // namespace core

#endif // NET_EVENTDISPATCHER_H
