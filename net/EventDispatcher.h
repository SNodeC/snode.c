/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <climits>
#include <ctime>
#include <list>
#include <map>
#include <tuple>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "EventReceiver.h"

namespace net {

    class EventLoop;

    template <typename EventReceiverT>
    class EventDispatcher {
    public:
        using EventReceiver = EventReceiverT;

        explicit EventDispatcher(fd_set& fdSet, long maxInactivity) // NOLINT(google-runtime-references)
            : fdSet(fdSet)
            , maxInactivity(maxInactivity) {
        }

        EventDispatcher(const EventDispatcher&) = delete;

        EventDispatcher& operator=(const EventDispatcher&) = delete;

    private:
        static bool contains(std::list<EventReceiver*>& events, EventReceiver*& event) {
            typename std::list<EventReceiver*>::iterator it = std::find(events.begin(), events.end(), event);

            return it != events.end();
        }

    public:
        void enable(EventReceiver* eventReceiver) {
            if (EventDispatcher<EventReceiver>::contains(disabledEventReceiver, eventReceiver)) {
                // same tick
                disabledEventReceiver.remove(eventReceiver);
            } else if (!eventReceiver->isEnabled() && !EventDispatcher<EventReceiver>::contains(enabledEventReceiver, eventReceiver)) {
                // normal
                enabledEventReceiver.push_back(eventReceiver);
                eventReceiver->enabled();
            }
        }

        void disable(EventReceiver* eventReceiver) {
            if (EventDispatcher<EventReceiver>::contains(enabledEventReceiver, eventReceiver)) {
                // same tick
                enabledEventReceiver.remove(eventReceiver);
                eventReceiver->disabled();
                eventReceiver->destructIfUnobserved();
            } else if (eventReceiver->isEnabled() && !EventDispatcher<EventReceiver>::contains(disabledEventReceiver, eventReceiver)) {
                // normal
                disabledEventReceiver.push_back(eventReceiver);
            }
        }

        long getTimeout() const {
            return maxInactivity;
        }

    private:
        int getLargestFd() {
            int fd = -1;

            if (!observedEvents.empty()) {
                fd = observedEvents.rbegin()->first;
            }

            return fd;
        }

        long observeEnabledEvents() {
            long nextTimeout = LONG_MAX;

            for (EventReceiver* eventReceiver : enabledEventReceiver) {
                int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
                observedEvents[fd].push_front(eventReceiver);
                FD_SET(fd, &fdSet);
                nextTimeout = std::min(nextTimeout, eventReceiver->getTimeout());
            }
            enabledEventReceiver.clear();

            return nextTimeout;
        }

        void unobserveDisabledEvents() {
            for (EventReceiver* eventReceiver : disabledEventReceiver) {
                int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
                observedEvents[fd].remove(eventReceiver);
                if (observedEvents[fd].empty()) {
                    observedEvents.erase(fd);
                    FD_CLR(fd, &fdSet);
                }
                eventReceiver->disabled();
                eventReceiver->destructIfUnobserved();
            }
            disabledEventReceiver.clear();
        }

        void unobserveObservedEvents() {
            for (auto& [fd, eventReceivers] : observedEvents) {
                for (EventReceiver* eventReceiver : eventReceivers) {
                    disabledEventReceiver.push_back(eventReceiver);
                }
            }
            unobserveDisabledEvents();
        }

    protected:
        long dispatchEvents(const fd_set& fdSet, int& counter, time_t currentTime) {
            long nextInactivityTimeout = LONG_MAX;

            for (const auto& [fd, eventReceivers] : observedEvents) {
                EventReceiver* eventReceiver = eventReceivers.front();
                long maxInactivity = eventReceiver->getTimeout();
                if (FD_ISSET(fd, &fdSet)) {
                    counter--;
                    dispatchEventTo(eventReceiver);
                    eventReceiver->lastTriggered = currentTime;
                    nextInactivityTimeout = std::min(nextInactivityTimeout, maxInactivity);
                } else {
                    long inactivity = currentTime - eventReceiver->lastTriggered;
                    if (inactivity >= maxInactivity) {
                        eventReceiver->disable();
                    } else {
                        nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                    }
                }
            }

            return nextInactivityTimeout;
        }

    private:
        virtual void dispatchEventTo(EventReceiver*) = 0;

        std::map<int, std::list<EventReceiver*>> observedEvents;

    private:
        std::list<EventReceiver*> enabledEventReceiver;
        std::list<EventReceiver*> disabledEventReceiver;

        fd_set& fdSet;

    protected:
        long maxInactivity;

        friend class EventLoop;
    };

} // namespace net

#endif // EVENTDISPATCHER_H
