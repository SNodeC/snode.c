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
#include <climits>
#include <ctime>
#include <list>
#include <map>

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
                //                eventReceiver->destructIfUnobserved();
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

            if (!observedEventReceiver.empty()) {
                fd = observedEventReceiver.rbegin()->first;
            }

            return fd;
        }

        long observeEnabledEvents() {
            long nextTimeout = LONG_MAX;

            for (EventReceiver* eventReceiver : enabledEventReceiver) {
                int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
                observedEventReceiver[fd].push_front(eventReceiver);
                FD_SET(fd, &fdSet);
                nextTimeout = std::min(nextTimeout, eventReceiver->getTimeout());
            }
            enabledEventReceiver.clear();

            return nextTimeout;
        }

        long dispatchActiveEvents(const fd_set& fdSet, int& counter, time_t currentTime) {
            long nextInactivityTimeout = LONG_MAX;

            for (const auto& [fd, eventReceivers] : observedEventReceiver) {
                EventReceiver* eventReceiver = eventReceivers.front();
                long maxInactivity = eventReceiver->getTimeout();
                if (FD_ISSET(fd, &fdSet)) {
                    counter--;
                    dispatchEventTo(eventReceiver);
                    eventReceiver->setLastTriggered(currentTime);
                    nextInactivityTimeout = std::min(nextInactivityTimeout, maxInactivity);
                } else {
                    long inactivity = currentTime - eventReceiver->getLastTriggered();
                    if (inactivity >= maxInactivity) {
                        eventReceiver->disable();
                    } else {
                        nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                    }
                }
            }

            return nextInactivityTimeout;
        }

        void unobserveDisabledEvents() {
            for (EventReceiver* eventReceiver : disabledEventReceiver) {
                int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty()) {
                    observedEventReceiver.erase(fd);
                    FD_CLR(fd, &fdSet);
                }
                eventReceiver->disabled();
                //                eventReceiver->destructIfUnobserved();
            }
            disabledEventReceiver.clear();
        }

        void disableObservedEvents() {
            for (auto& [fd, eventReceivers] : observedEventReceiver) {
                for (EventReceiver* eventReceiver : eventReceivers) {
                    disabledEventReceiver.push_back(eventReceiver);
                }
            }
        }

        virtual void dispatchEventTo(EventReceiver*) = 0;

        std::map<int, std::list<EventReceiver*>> observedEventReceiver;

        std::list<EventReceiver*> enabledEventReceiver;
        std::list<EventReceiver*> disabledEventReceiver;

        fd_set& fdSet;

        long maxInactivity;

        friend class EventLoop;
    };

} // namespace net

#endif // EVENTDISPATCHER_H
